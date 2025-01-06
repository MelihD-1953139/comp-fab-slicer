#include "slicer.h"
#include "model.h"
#include "utils.h"

#include <algorithm>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.engine.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>
#include <cstdint>
#include <glm/trigonometric.hpp>
#include <hfs/hfs_format.h>
#include <memory>
#include <numbers>
#include <sys/types.h>

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
  m_lineWidth = MM2INT(nozzleDiameter);
  m_shift = m_lineWidth / 2;
}

void Slicer::createSlices() {
  m_slices.clear();

  for (size_t i = 0; i < m_layerCount; ++i) {
    auto sliceHeight = m_layerHeight / 2.0f + m_layerHeight * i + 1e-15;
    m_slices.push_back(m_model->getSlice(sliceHeight));
  }
}

void Slicer::createWalls(int wallCount) {
  for (auto &slice : m_slices) {
    auto objectPerimeter = toPaths64(slice.getPerimeter());
    slice.clear();

    for (size_t j = 0; j < wallCount; ++j) {
      double delta = -static_cast<double>(m_lineWidth) / 2.0 -
                     static_cast<double>(m_lineWidth * j);
      Paths64 wall = InflatePaths(objectPerimeter, delta, JoinType::Round,
                                  EndType::Polygon);
      if (j == 0)
        slice.addOuterWall(toPathsD(closePaths(wall)));
      else
        slice.addInnerWall(toPathsD(closePaths(wall)));
    }
  }
}

void Slicer::createFill(FillType fillType, int floorCount, int roofCount) {
  if (fillType == NoFill)
    return;

  floorCount = std::clamp<int>(floorCount, 0, m_layerCount);
  roofCount = std::clamp<int>(roofCount, 0, m_layerCount - floorCount);

  // First `floorCount` layers are always filled
  for (size_t i = 0; i < floorCount; ++i) {
    auto &slice = m_slices[i];
    Paths64 fill;
    m_currentArea = toPaths64(slice.getInnermostShell());
    generateFill(fill, fillType, i % 2 == 0 ? 45.0 : 135.0);
    slice.addFill(toPathsD(fill));
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
    m_currentArea = toPaths64(floorArea);
    Paths64 floor;
    generateFill(floor, fillType, i % 2 == 0 ? 45.0 : 135.0);

    PathsD roofArea;
    if (i + 1 >= m_layerCount)
      roofArea = PathsD();
    else
      roofArea = m_slices[i + 1].getInnermostShell();

    for (size_t j = 2; j <= roofCount; ++j) {
      roofArea = Intersect(roofArea, m_slices[i + j].getInnermostShell(),
                           FillRule::EvenOdd);
    }
    roofArea =
        Difference(slice.getInnermostShell(), roofArea, FillRule::EvenOdd);
    m_currentArea = toPaths64(roofArea);
    Paths64 roof;
    generateFill(roof, fillType, i % 2 == 0 ? 45.0 : 135.0);

    slice.addFill(toPathsD(floor));
    slice.addFill(toPathsD(roof));

    auto fillArea = Union(floorArea, roofArea, FillRule::NonZero);
    slice.setFillArea(fillArea);
  }

  // Last `roofCount` layers are always filled
  for (size_t i = m_layerCount - roofCount; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    m_currentArea = toPaths64(slice.getInnermostShell());
    Paths64 fill;
    generateFill(fill, fillType, i % 2 == 0 ? 45.0 : 135.0);
    slice.addFill(toPathsD(fill));
    slice.setFillArea(toPathsD(m_currentArea));
  }
}

void Slicer::generateFill(Paths64 &fillResult, FillType fillType,
                          const double angle) {
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
  if (infillType == NoInfill)
    return;
  for (m_currentLayer = 0; m_currentLayer < m_slices.size(); ++m_currentLayer) {
    m_currentArea = toPaths64(
        Difference(m_slices[m_currentLayer].getInnermostShell(),
                   m_slices[m_currentLayer].getFillArea(), FillRule::NonZero));

    Paths64 infill;
    switch (infillType) {
    case NoInfill:
    case InfillCount:
      return;
    case LinesInfill:
      generateLineInfill(infill, getLineDistance(1, density),
                         m_currentLayer % 2 == 0 ? 45.0f : 135.0f, 0);
      break;
    case GridInfill:
      generateGridInfill(infill, getLineDistance(2, density), 45.0);
      break;
    case Cubic:
      generateCubicInfill(infill, getLineDistance(3, density), 45.0);
      break;
    case Triangle:
      generateTriangleInfill(infill, getLineDistance(3, density), 45.0);
      break;
    case TriHexagon:
      generateTriHexagonInfill(infill, getLineDistance(3, density), 45.0);
      break;
    case Tetrahedral:
      generateTetrahedralInfill(infill, getLineDistance(2, density));
      break;
    case QuarterCubic:
      generateQuarterCubicInfill(infill, getLineDistance(2, density));
      break;
    case ConcentricInfill:
      generateConcentricInfill(infill, getLineDistance(1, density));
      break;
    }
    m_slices[m_currentLayer].addInfill(toPathsD(infill));
  }
}

void Slicer::createSupport(SupportType supportType, float density,
                           size_t wallCount, size_t brimCount) {
  if (supportType == NoSupport)
    return;
  m_slices.back().setSupportArea(PathsD());
  for (auto it = m_slices.rbegin() + 1; it < m_slices.rend(); ++it) {
    auto previousSliceIT = it - 1;

    Paths64 prevPerimAndSupport = toPaths64(previousSliceIT->getPerimeter());
    prevPerimAndSupport.append_range(
        toPaths64(previousSliceIT->getSupportArea()));
    prevPerimAndSupport = Union(prevPerimAndSupport, FillRule::EvenOdd);

    auto dilatedPerimeter =
        InflatePaths(toPaths64(it->getPerimeter()), m_lineWidth * 2.0f,
                     JoinType::Miter, EndType::Polygon);

    auto supportArea =
        Difference(prevPerimAndSupport, dilatedPerimeter, FillRule::EvenOdd);
    it->setSupportArea(toPathsD(supportArea));

    // Remove support from the last layer before a floor
    auto lastLayerSupport =
        Difference(toPaths64(previousSliceIT->getPerimeter()),
                   toPaths64(it->getPerimeter()), FillRule::EvenOdd);
    supportArea = Difference(supportArea, lastLayerSupport, FillRule::EvenOdd);

    // Horizontal expansion of the support
    supportArea = InflatePaths(supportArea, 2.0 * m_lineWidth, JoinType::Round,
                               EndType::Polygon);
    supportArea = Difference(supportArea, dilatedPerimeter, FillRule::EvenOdd);

    m_currentArea = supportArea;

    Paths64 support;
    for (size_t i = 0; i < (it != m_slices.rend() - 1 ? wallCount : brimCount);
         ++i) {
      m_currentArea =
          InflatePaths(supportArea, -static_cast<double>(m_lineWidth) * i,
                       JoinType::Round, EndType::Polygon);
      support.append_range(closePaths(m_currentArea));
    }
    Paths64 supportLines;
    switch (supportType) {
    case NoSupport:
    case SupportCount:
      return;
    case LinesSupport:
      generateLineInfill(supportLines, m_lineWidth / density, 0.0, 0.0);
      break;
    case GridSupport:
      generateGridInfill(supportLines, (2.0 * m_lineWidth) / density, 0.0);
      break;
    case Triangles:
      generateTriangleInfill(supportLines, (3.0 * m_lineWidth) / density, 0.0);
      break;
    case ConcentricSupport:
      generateConcentricInfill(supportLines, m_lineWidth / density);
      break;
    }

    Clipper64 clipper;
    clipper.AddClip(supportArea);
    clipper.AddOpenSubject(supportLines);
    Paths64 discard;
    clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard,
                    supportLines);

    support.append_range(supportLines);
    it->addSupport(toPathsD(support));
  }
}

void Slicer::createBrim(BrimLocation brimLocation, int lineCount) {
  auto &slice = m_slices.front();
  const Paths64 &perimeter = toPaths64(slice.getPerimeter());
  slice.removeSupport();

  for (int i = 1; i <= lineCount; ++i) {

    Paths64 brim = InflatePaths(perimeter, m_lineWidth * i, JoinType::Round,
                                EndType::Polygon);

    auto it = std::remove_if(brim.begin(), brim.end(),
                             [&](const Path64 &path) -> bool {
                               switch (brimLocation) {
                               case Outside:
                                 return !IsPositive(path);
                               case Inside:
                                 return IsPositive(path);
                               case Both:
                               default:
                                 return false;
                               }
                             });
    brim.erase(it, brim.end());

    slice.addSupport(toPathsD(closePaths(brim)));
  }
}

void Slicer::createSkirt(int lineCount, int height, float distance) {

  height = std::min(height, (int)m_slices.size());

  // First layer gets `lineCount` lines
  auto &slice = m_slices.front();
  auto perimeter = slice.getPerimeter();
  PathsD area = perimeter;
  for (auto &support : slice.getSupport())
    area = Union(perimeter, support, FillRule::NonZero);

  auto skirt = InflatePaths(area, distance, JoinType::Round, EndType::Polygon);

  for (int i = 0; i < lineCount; ++i) {
    slice.addSupport(closePaths(InflatePaths(
        skirt, INT2MM(i * m_lineWidth), JoinType::Round, EndType::Polygon)));
  }

  // `height` - 1 layers get the first skirt aswell
  for (auto sliceIt = m_slices.begin() + 1; sliceIt < m_slices.begin() + height;
       ++sliceIt) {
    sliceIt->addSupport(closePaths(skirt));
  }
}

int64_t Slicer::getLineDistance(uint lineCount, float density) const {
  return MM2INT(INT2MM(m_lineWidth * lineCount) / density);
}

int64_t Slicer::getShiftOffsetFromInfillOriginAndRotation(const Paths64 &area,
                                                          const double angle) {
  Rect64 bounds = GetBounds(area);
  Point64 origin = bounds.MidPoint();
  // Nexus::Logger::debug("shift offset origin point: ({}, {})",
  // INT2MM(origin.x),
  //                      INT2MM(origin.y));
  if (origin.x != 0 || origin.y != 0) {
    return origin.x * std::cos(glm::radians(angle)) -
           origin.y * std::sin(glm::radians(angle));
  }
  return 0;
}

void Slicer::generateLineInfill(Paths64 &infillResult,
                                const int64_t lineDistance, const double angle,
                                int64_t shift) {
  if (lineDistance == 0 || m_currentArea.empty())
    return;

  Paths64 outline = m_currentArea;

  rotatePaths(outline, angle);

  const int64_t origShift = shift;
  const int64_t rotShift =
      getShiftOffsetFromInfillOriginAndRotation(m_currentArea, angle);
  shift += rotShift + m_shift + extraShift;
  if (shift < 0) {
    shift = lineDistance - (-shift) % lineDistance;
  } else {
    shift = shift % lineDistance;
  }

  auto [minX, minY, maxX, maxY] = GetBounds(outline);

  Paths64 lines;
  int64_t y = minY - shift;
  while (y < maxY + shift) {
    lines.push_back({{minX, y}, {maxX, y}});
    y += lineDistance;
  }
  unRotatePaths(lines, angle);

  Clipper64 clipper;
  clipper.AddOpenSubject(lines);
  clipper.AddClip(m_currentArea);
  Paths64 discard;
  clipper.Execute(ClipType::Intersection, FillRule::NonZero, discard, lines);
  infillResult.append_range(lines);
}

void Slicer::generateGridInfill(Paths64 &infillResult,
                                const int64_t lineDistance,
                                const double angle) {
  generateLineInfill(infillResult, lineDistance, 0, 0);
  generateLineInfill(infillResult, lineDistance, 90, -1000);
}

void Slicer::generateCubicInfill(Paths64 &infillResult,
                                 const int64_t lineDistance,
                                 const double angle) {
  const int64_t shift =
      (m_currentLayer * MM2INT(m_layerHeight)) / std::numbers::sqrt2;
  generateLineInfill(infillResult, lineDistance, angle + 0, shift);
  generateLineInfill(infillResult, lineDistance, angle + 120, shift);
  generateLineInfill(infillResult, lineDistance, angle + 240, shift);
}

void Slicer::generateTriangleInfill(Paths64 &infillResult,
                                    const int64_t lineDistance,
                                    const double angle) {
  generateLineInfill(infillResult, lineDistance, angle, 0);
  generateLineInfill(infillResult, lineDistance, angle + 60, 0);
  generateLineInfill(infillResult, lineDistance, angle + 120, 0);
}
void Slicer::generateTriHexagonInfill(Paths64 &infillResult,
                                      const int64_t lineDistance,
                                      const double angle) {
  generateLineInfill(infillResult, lineDistance, angle, 0);
  generateLineInfill(infillResult, lineDistance, angle + 60, 0);
  generateLineInfill(infillResult, lineDistance, angle + 120,
                     lineDistance / 2.0f);
}

void Slicer::generateTetrahedralInfill(Paths64 &infillResult,
                                       const int64_t lineDistance) {
  generateHalfTetrahedralInfill(infillResult, lineDistance, 0, 0.0);
  generateHalfTetrahedralInfill(infillResult, lineDistance, 90, 0.0);
}
void Slicer::generateQuarterCubicInfill(Paths64 &infillResult,
                                        const int64_t lineDistance) {
  generateHalfTetrahedralInfill(infillResult, lineDistance, 0, 0.0);
  generateHalfTetrahedralInfill(infillResult, lineDistance, 90, 0.5);
}

void Slicer::generateHalfTetrahedralInfill(Paths64 &infillResult,
                                           const int64_t lineDistance,
                                           const double angle, double zShift) {
  int64_t period = lineDistance * 2;
  int64_t shift = m_currentLayer * MM2INT(m_layerHeight) + zShift * period * 2;
  shift /= std::numbers::sqrt2;
  shift %= period;
  shift = std::min(shift, period - shift);
  shift = std::min(shift, period / 2 - m_lineWidth / 2);
  shift = std::max(shift, m_lineWidth / 2);
  generateLineInfill(infillResult, period, 45.0 + angle, shift);
  generateLineInfill(infillResult, period, 45.0 + angle, -shift);
}

void Slicer::generateConcentricInfill(Paths64 &infillResult,
                                      const int64_t lineDistance) {
  std::vector<Paths64> fills{closePaths(InflatePaths(
      m_currentArea, -lineDistance, JoinType::Round, EndType::Polygon))};
  while (fills.back().size() > 0) {
    fills.push_back(closePaths(InflatePaths(
        fills.back(), -lineDistance, JoinType::Round, EndType::Polygon)));
  }

  for (auto &paths : fills)
    infillResult.append_range(paths);
}