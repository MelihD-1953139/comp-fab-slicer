#include "slicer.h"
#include "Nexus/Log.h"
#include "model.h"
#include "utils.h"

#include <algorithm>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.engine.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>
#include <glm/trigonometric.hpp>
#include <hfs/hfs_format.h>
#include <memory>

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

  floorCount = std::clamp<int>(floorCount, 0, m_layerCount);
  roofCount = std::clamp<int>(roofCount, 0, m_layerCount - floorCount);

  // First `floorCount` layers are always filled
  for (size_t i = 0; i < floorCount; ++i) {
    auto &slice = m_slices[i];
    PathsD fill;
    m_currentArea = slice.getInnermostShell();
    generateFill(fill, fillType, i % 2 == 0 ? 45.0f : -45.0f);
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
    m_currentArea = floorArea;
    PathsD floor;
    generateFill(floor, fillType, i % 2 == 0 ? 45.0f : -45.0f);

    PathsD roofArea = m_slices[i + 1].getInnermostShell();
    for (size_t j = 2; j <= roofCount; ++j) {
      roofArea = Intersect(roofArea, m_slices[i + j].getInnermostShell(),
                           FillRule::EvenOdd);
    }
    roofArea =
        Difference(slice.getInnermostShell(), roofArea, FillRule::EvenOdd);
    m_currentArea = roofArea;
    PathsD roof;
    generateFill(roof, fillType, i % 2 == 0 ? 45.0f : -45.0f);

    slice.addFill(floor);
    slice.addFill(roof);

    auto fillArea = Union(floorArea, roofArea, FillRule::NonZero);
    slice.setFillArea(fillArea);
  }

  // Last `roofCount` layers are always filled
  for (size_t i = m_layerCount - roofCount; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    m_currentArea = slice.getInnermostShell();
    PathsD fill;
    generateFill(fill, fillType, i % 2 == 0 ? 45.0f : -45.0f);
    slice.addFill(fill);
    slice.setFillArea(m_currentArea);
  }
}

void Slicer::generateFill(PathsD &fillResult, FillType fillType,
                          const float angle) {
  switch (fillType) {
  case NoFill:
  case FillCount:
    return;
  case ConcentricFill:
    generateConcentricInfill(fillResult, m_lineWidth);
    break;
  case LinesFill:
    generateLineInfill(fillResult, m_lineWidth, angle, 0);
    break;
  }
}

void Slicer::createInfill(InfillType infillType, float density) {
  for (m_currentLayer = 0; m_currentLayer < m_slices.size(); ++m_currentLayer) {
    m_currentArea =
        Difference(m_slices[m_currentLayer].getInnermostShell(),
                   m_slices[m_currentLayer].getFillArea(), FillRule::NonZero);

    PathsD infill;
    switch (infillType) {
    case NoInfill:
    case InfillCount:
      return;
    case LinesInfill:
      generateLineInfill(infill, m_lineWidth / density,
                         m_currentLayer % 2 == 0 ? 45.0f : 135.0f, 0);
      break;
    case Grid:
      generateGridInfill(infill, (2.0f * m_lineWidth) / density, 45.0f, 0);
      break;
    case Cubic:
      generateCubicInfill(infill, (3.0f * m_lineWidth) / density, 45.0f);
      break;
    case Triangle:
      generateTriangleInfill(infill, (3.0f * m_lineWidth) / density, 45.0f, 0);
      break;
    case TriHexagon:
      generateTriHexagonInfill(infill, (3.0f * m_lineWidth) / density, 45.0f,
                               0);
      break;
    case Tetrahedral:
      generateTetrahedralInfill(infill, (2.0 * m_lineWidth) / density);
      break;
    case QuarterCubic:
      generateQuarterCubicInfill(infill, (2.0 * m_lineWidth) / density);
      break;
    case HalfTetrahedral:
      generateHalfTetrahedralInfill(infill, (2.0 * m_lineWidth) / density, 45.0,
                                    0);
      break;
    case ConcentricInfill:
      generateConcentricInfill(infill, m_lineWidth / density);
      break;
    }
    m_slices[m_currentLayer].addInfill(infill);
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
      m_infillLineDistance = m_lineWidth;
    } else {
      m_infillLineDistance = (2.0 * m_lineWidth) / density;
    }

    PathsD support;
    generateGridInfill(support, (2.0 * m_lineWidth) / density, 0.0, 0.0f);
    ClipperD clipper;
    clipper.AddClip(supportArea);
    clipper.AddOpenSubject(support);
    PathsD discard;
    clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard,
                    support);
    it->addSupport(support);
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

void Slicer::generateLineInfill(PathsD &infillResult, const double lineDistance,
                                const float angle, float shift) {
  if (lineDistance == 0 || m_currentArea.empty())
    return;

  PathsD outline = m_currentArea;
  auto bounds = GetBounds(outline);
  double diagonal = std::sqrt(bounds.Width() * bounds.Width() +
                              bounds.Height() * bounds.Height());
  outline = InflatePaths(outline, diagonal, JoinType::Miter, EndType::Polygon);
  rotatePaths(outline, angle);

  shift += getShiftOffsetFromInfillOriginAndRotation(m_currentArea, angle) +
           lineDistance;
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
  unRotatePaths(lines, angle);

  ClipperD clipper;
  clipper.AddOpenSubject(lines);
  clipper.AddClip(m_currentArea);
  PathsD discard;
  clipper.Execute(ClipType::Intersection, FillRule::NonZero, discard, lines);
  infillResult.append_range(lines);
}

void Slicer::generateGridInfill(PathsD &infillResult, const double lineDistance,
                                const float angle, float shift) {
  generateLineInfill(infillResult, lineDistance, angle + 0, 0);
  generateLineInfill(infillResult, lineDistance, angle + 90, 0);
}

void Slicer::generateCubicInfill(PathsD &infillResult,
                                 const double lineDistance, const float angle) {
  const double shift = (m_currentLayer * m_layerHeight) / std::sqrt(2);
  generateLineInfill(infillResult, lineDistance, angle + 0, shift);
  generateLineInfill(infillResult, lineDistance, angle + 120, shift);
  generateLineInfill(infillResult, lineDistance, angle + 240, shift);
}

void Slicer::generateTriangleInfill(PathsD &infillResult,
                                    const double lineDistance,
                                    const float angle, float shift) {
  generateLineInfill(infillResult, lineDistance, angle, 0);
  generateLineInfill(infillResult, lineDistance, angle + 60, 0);
  generateLineInfill(infillResult, lineDistance, angle + 120, 0);
}
void Slicer::generateTriHexagonInfill(PathsD &infillResult,
                                      const double lineDistance,
                                      const float angle, float shift) {
  generateLineInfill(infillResult, lineDistance, angle, 0);
  generateLineInfill(infillResult, lineDistance, angle + 60, 0);
  generateLineInfill(infillResult, lineDistance, angle + 120,
                     lineDistance / 2.0f);
}

void Slicer::generateTetrahedralInfill(PathsD &infillResult,
                                       const double lineDistance) {
  generateHalfTetrahedralInfill(infillResult, lineDistance, 0, 0.0);
  generateHalfTetrahedralInfill(infillResult, lineDistance, 90, 0.0);
}
void Slicer::generateQuarterCubicInfill(PathsD &infillResult,
                                        const double lineDistance) {
  generateHalfTetrahedralInfill(infillResult, lineDistance, 0, 0.0);
  generateHalfTetrahedralInfill(infillResult, lineDistance, 90, 0.5);
}

void Slicer::generateHalfTetrahedralInfill(PathsD &infillResult,
                                           const double lineDistance,
                                           const float angle, float zShift) {
  double period = lineDistance * 2.0f;
  double shift = std::fmod(
      ((m_currentLayer * m_layerHeight + zShift * period * 2) / std::sqrt(2)),
      period);
  shift = std::min(shift, period - shift);
  shift = std::min(shift, period / 2.0 - m_lineWidth / 2.0);
  shift = std::max(shift, m_lineWidth / 2.0);
  generateLineInfill(infillResult, period, 45.0 + angle, shift);
  generateLineInfill(infillResult, period, 45.0 + angle, -shift);
}

void Slicer::generateConcentricInfill(PathsD &infillResult,
                                      const double lineDistance) {
  std::vector<PathsD> fills{closePathsD(InflatePaths(
      m_currentArea, -lineDistance, JoinType::Round, EndType::Polygon))};
  while (fills.back().size() > 0) {
    fills.push_back(closePathsD(InflatePaths(
        fills.back(), -lineDistance, JoinType::Round, EndType::Polygon)));
  }

  for (auto &paths : fills)
    infillResult.append_range(paths);
}