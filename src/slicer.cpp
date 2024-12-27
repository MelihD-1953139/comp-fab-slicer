#include "slicer.h"
#include "glm/trigonometric.hpp"
#include "model.h"
#include "utils.h"

#include <algorithm>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.engine.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>
#include <hfs/hfs_format.h>
#include <memory>
#include <os/lock.h>

using namespace Clipper2Lib;

Slicer::Slicer(const char *modelPath)
    : m_model(std::make_unique<Model>(modelPath)) {}

void Slicer::loadModel(const char *modelPath) {
  m_model = std::make_unique<Model>(modelPath);
  m_slices.clear();
}

void Slicer::init(float layerHeight, float nozzleDiameter) {
  m_layerCount = m_model->getLayerCount(layerHeight);
  m_layerHeight = layerHeight;
  m_lineWidth = nozzleDiameter;
}

void Slicer::createSlices() {
  m_slices.clear();

  for (size_t i = 0; i < m_layerCount; ++i) {
    auto sliceHeight = m_layerHeight / 2.0f + m_layerHeight * i + 1e-15;
    m_slices.push_back(m_model->getSlice(sliceHeight));
  }
}

void Slicer::createWalls(int wallCount) {
  for (size_t i = 0; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    auto objectPerimeter = slice.getPerimeter();
    slice.clear();

    for (size_t j = 0; j < wallCount; ++j) {
      float delta = -m_lineWidth / 2.0f - m_lineWidth * j;
      PathsD wall = InflatePaths(objectPerimeter, delta, JoinType::Round,
                                 EndType::Polygon);
      slice.addShell(closePathsD(wall));
    }
  }
}

void Slicer::createFill(FillType fillType, int floorCount, int roofCount) {
  // m_floorCount = floorCount;
  // m_roofCount = roofCount;
  floorCount = std::clamp<int>(floorCount, 0, m_layerCount);
  roofCount = std::clamp<int>(roofCount, 0, m_layerCount - floorCount);

  auto fillFunc = generateConcentricFill;
  switch (fillType) {
  case Concentric:
    fillFunc = generateConcentricFill;
    break;
  case LinesFill:
    fillFunc = generateLineFill;
    break;
  default:
    return;
  }

  // First `floorCount` layers are always filled
  for (size_t i = 0; i < floorCount; ++i) {
    auto &slice = m_slices[i];
    auto fill = fillFunc(m_lineWidth, slice.getInnermostShell(),
                         i % 2 == 0 ? 45.0f : -45.0f);
    slice.addFill(fill);
    slice.setFillArea(slice.getInnermostShell());
  }

  for (size_t i = floorCount; i < m_layerCount - roofCount; ++i) {
    auto &slice = m_slices[i];

    // Find floor sections;
    PathsD floorArea = m_slices[i - 1].getInnermostShell();
    for (size_t j = 2; j <= floorCount; ++j) {
      floorArea = Intersect(floorArea, m_slices[i - j].getInnermostShell(),
                            FillRule::EvenOdd);
    }
    floorArea =
        Difference(slice.getInnermostShell(), floorArea, FillRule::EvenOdd);
    PathsD floor =
        fillFunc(m_lineWidth, floorArea, i % 2 == 0 ? 45.0f : -45.0f);

    PathsD roofArea = m_slices[i + 1].getInnermostShell();
    for (size_t j = 2; j <= roofCount; ++j) {
      roofArea = Intersect(roofArea, m_slices[i + j].getInnermostShell(),
                           FillRule::EvenOdd);
    }
    roofArea =
        Difference(slice.getInnermostShell(), roofArea, FillRule::EvenOdd);
    PathsD roof = fillFunc(m_lineWidth, roofArea, i % 2 == 0 ? 45.0f : -45.0f);

    slice.addFill(floor);
    slice.addFill(roof);

    auto fillArea = Union(floorArea, roofArea, FillRule::NonZero);
    slice.setFillArea(fillArea);
  }

  // Last `roofCount` layers are always filled
  for (size_t i = m_layerCount - roofCount; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    auto fill = fillFunc(m_lineWidth, slice.getInnermostShell(),
                         i % 2 == 0 ? 45.0f : -45.0f);
    slice.addFill(fill);
    slice.setFillArea(slice.getInnermostShell());
  }
}

void Slicer::createInfill(InfillType infillType, float density) {
  auto infillFunc = &Slicer::generateGridInfill;
  float lineDistance;
  float angleEven;
  float angleOdd;

  switch (infillType) {
  case NoInfill:
  case InfillCount:
    return;
  case LinesInfill:
    lineDistance = m_lineWidth / density;
    infillFunc = &Slicer::generateLineInfill;
    angleEven = 45.0f;
    angleOdd = -45.0f;
    break;
  case Grid:
    infillFunc = &Slicer::generateGridInfill;
    lineDistance = (2.0f * m_lineWidth) / density;
    angleEven = angleOdd = 45.0f;
    break;
  case Triangle:
    infillFunc = &Slicer::generateTriangleInfill;
    lineDistance = (3.0f * m_lineWidth) / density;
    angleEven = angleOdd = 45.0f;
    break;
  case TriHexagon:
    infillFunc = &Slicer::generateTriHexagonInfill;
    lineDistance = (3.0f * m_lineWidth) / density;
    angleEven = angleOdd = 45.0f;
    break;
  }

  for (int i = 0; i < m_slices.size(); ++i) {
    auto infillArea = Difference(m_slices[i].getInnermostShell(),
                                 m_slices[i].getFillArea(), FillRule::EvenOdd);

    PathsD infill;
    (this->*infillFunc)(infill, infillArea, lineDistance,
                        i % 2 == 0 ? angleEven : angleOdd, 0.0f);
    m_slices[i].addInfill(infill);
  }
}

void Slicer::createSupport(float density) {
  for (auto it = m_slices.rbegin() + 1; it < m_slices.rend(); ++it) {
    auto previousSliceIT = it - 1;
    auto prevPerimAndSupport =
        Union(previousSliceIT->getPerimeter(),
              previousSliceIT->getSupportArea(), FillRule::EvenOdd);

    auto dilatedPerimeter = InflatePaths(it->getPerimeter(), m_lineWidth * 3.0f,
                                         JoinType::Round, EndType::Polygon);

    auto supportArea =
        Difference(prevPerimAndSupport, dilatedPerimeter, FillRule::EvenOdd);

    auto lastLayerSupport = Difference(previousSliceIT->getPerimeter(),
                                       it->getPerimeter(), FillRule::EvenOdd);

    supportArea = Difference(supportArea, lastLayerSupport, FillRule::EvenOdd);

    it->setSupportArea(supportArea);

    if (it == m_slices.rend() - 1) {
      auto support = generateConcentricFill(m_lineWidth, supportArea);
      it->addSupport(closePathsD(support));
    } else {
      auto support = generateSparseRectangleInfill(m_lineWidth, density,
                                                   supportArea, 0.0f);
      ClipperD clipper;
      clipper.AddClip(supportArea);
      clipper.AddOpenSubject(support);
      PathsD discard;
      clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard,
                      support);
      it->addSupport(support);
    }
  }
}

void Slicer::createBrim(BrimLocation brimLocation, int lineCount) {
  auto &slice = m_slices.front();
  const PathsD &perimeter = slice.getPerimeter();

  for (int i = 1; i <= lineCount; ++i) {

    PathsD brim = InflatePaths(perimeter, m_lineWidth * i, JoinType::Round,
                               EndType::Polygon);

    auto it = std::remove_if(brim.begin(), brim.end(),
                             [&](const PathD &path) -> bool {
                               switch (brimLocation) {
                               case Outside:
                                 return !IsPositive(path);
                               case Inside:
                                 return IsPositive(path);
                               case Both:
                                 return false;
                               default:
                                 return false;
                               }
                             });
    brim.erase(it, brim.end());

    slice.addSupport(closePathsD(brim));
  }
}

PathsD getOutermost(const PathsD &paths) {
  if (paths.empty())
    return PathsD();

  auto outermostPath =
      std::max_element(paths.begin(), paths.end(),
                       [](const PathD &lhs, const PathD &rhs) -> bool {
                         return Area(lhs) < Area(rhs);
                       });
  return {*outermostPath};
}
void Slicer::createSkirt(int lineCount, int height, float distance) {

  height = std::min(height, (int)m_slices.size());

  // First layer gets `lineCount` lines
  auto &slice = m_slices.front();
  auto perimeter = slice.getPerimeter();
  auto outerMostPerimeter = getOutermost(perimeter);
  for (int i = 0; i < lineCount; ++i) {
    slice.addSupport(
        closePathsD(InflatePaths(outerMostPerimeter, distance + i * m_lineWidth,
                                 JoinType::Round, EndType::Polygon)));
  }

  auto skirt = InflatePaths(outerMostPerimeter, distance, JoinType::Round,
                            EndType::Polygon);
  // `height` - 1 layers get the first skirt aswell
  for (auto sliceIt = m_slices.begin() + 1; sliceIt < m_slices.begin() + height;
       ++sliceIt) {
    sliceIt->addSupport(skirt);
  }
}

double Slicer::getShiftOffsetFromInfillOriginAndRotation(const PathsD &area,
                                                         const float angle) {
  auto bounds = GetBounds(area);
  auto origin = bounds.MidPoint();
  if (origin.x != 0 || origin.y != 0) {
    return origin.x * std::cos(glm::radians(angle)) -
           origin.y * std::sin(glm::radians(angle));
  }
  return 0;
}

void Slicer::generateLineInfill(PathsD &infillResult, const PathsD &area,
                                const double lineDistance, const float angle,
                                float shift) {
  if (lineDistance == 0 || area.empty())
    return;

  PathsD outline = area;
  auto bounds = GetBounds(outline);
  double diagonal = std::sqrt(bounds.Width() * bounds.Width() +
                              bounds.Height() * bounds.Height());
  outline = InflatePaths(outline, diagonal, JoinType::Miter, EndType::Polygon);
  rotatePaths(outline, angle);

  shift +=
      getShiftOffsetFromInfillOriginAndRotation(area, angle) + lineDistance;
  if (shift <= 0) {
    shift = lineDistance - std::fmod(-shift, lineDistance);
  } else {
    shift = std::fmod(shift, lineDistance);
  }

  auto [minX, minY, maxX, maxY] = GetBounds(outline);

  PathsD lines;
  double y = minY - shift;
  while (y < maxY - shift) {
    lines.push_back({{minX - shift, y}, {maxX - shift, y}});
    y += lineDistance;
  }
  rotatePaths(lines, -angle);

  ClipperD clipper;
  clipper.AddOpenSubject(lines);
  clipper.AddClip(area);
  PathsD discard;
  clipper.Execute(ClipType::Intersection, FillRule::NonZero, discard, lines);

  infillResult.append_range(lines);
}

void Slicer::generateGridInfill(PathsD &infillResult, const PathsD &area,
                                const double lineDistance, const float angle,
                                float shift) {
  generateLineInfill(infillResult, area, lineDistance, angle + 0);
  generateLineInfill(infillResult, area, lineDistance, angle + 90);
}

void Slicer::generateTriangleInfill(PathsD &infillResult, const PathsD &area,
                                    const double lineDistance,
                                    const float angle, float shift) {
  generateLineInfill(infillResult, area, lineDistance, angle);
  generateLineInfill(infillResult, area, lineDistance, angle + 60);
  generateLineInfill(infillResult, area, lineDistance, angle + 120);
}
void Slicer::generateTriHexagonInfill(PathsD &infillResult, const PathsD &area,
                                      const double lineDistance,
                                      const float angle, float shift) {
  generateLineInfill(infillResult, area, lineDistance, angle);
  generateLineInfill(infillResult, area, lineDistance, angle + 60);
  generateLineInfill(infillResult, area, lineDistance, angle + 120,
                     lineDistance / 2.0f);
}