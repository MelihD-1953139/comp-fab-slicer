#pragma once

#include "clipper2/clipper.core.h"
#include "clipper2/clipper.h"
#include "glm/ext/matrix_transform.hpp"
#include "model.h"
#include "slice.h"
#include "utils.h"

#include <vector>

inline void rotatePaths(Clipper2Lib::PathsD &paths, float angle) {
  auto [minx, miny, maxx, maxy] = GetBounds(paths);
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(
      model, glm::vec3((minx + maxx) / 2.0f, 0.0f, (miny + maxy) / 2.0f));
  model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::translate(
      model, -glm::vec3((minx + maxx) / 2.0f, 0.0f, (miny + maxy) / 2.0f));
  for (auto &path : paths) {
    for (auto &point : path) {
      glm::vec4 p = {point.x, 0.0f, point.y, 1.0f};
      p = model * p;
      point.x = p.x;
      point.y = p.z;
    }
  }
}

inline Clipper2Lib::PathsD
generateConcentricFill(float nozzleDiameter,
                       const Clipper2Lib::PathsD &lastShell,
                       float angle = 0.0f) {
  std::vector<Clipper2Lib::PathsD> fills{closePathsD(
      InflatePaths(lastShell, -nozzleDiameter, Clipper2Lib::JoinType::Round,
                   Clipper2Lib::EndType::Polygon))};
  while (fills.back().size() > 0) {
    fills.push_back(closePathsD(InflatePaths(fills.back(), -nozzleDiameter,
                                             Clipper2Lib::JoinType::Round,
                                             Clipper2Lib::EndType::Polygon)));
  }

  Clipper2Lib::PathsD fill;
  for (auto &paths : fills)
    fill.append_range(paths);

  return fill;
}

inline Clipper2Lib::PathsD
generateLineFill(float nozzleDiameter, const Clipper2Lib::PathsD &perimeter,
                 float angle = 45.0f) {
  using namespace Clipper2Lib;

  auto inflatedPerimeter = InflatePaths(perimeter, -nozzleDiameter,
                                        JoinType::Round, EndType::Polygon);

  auto [minX, minY, maxX, maxY] = GetBounds(inflatedPerimeter);
  double diagonal = distance({minX, minY}, {maxX, maxY});
  auto [rotatedMinX, rotatedMinY, rotatedMaxX, rotatedMaxY] =
      GetBounds(InflatePaths(inflatedPerimeter, diagonal, JoinType::Miter,
                             EndType::Polygon));

  PathsD lines;

  for (double y = rotatedMinY; y < rotatedMaxY; y += nozzleDiameter) {
    lines.push_back({{rotatedMinX, y}, {rotatedMaxX, y}});
  }

  rotatePaths(lines, angle);
  ClipperD clipper;
  clipper.AddClip(inflatedPerimeter);
  clipper.AddOpenSubject(lines);
  PathsD discard;
  clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard, lines);
  PathsD result = closePathsD(inflatedPerimeter);
  result.append_range(lines);
  return result;
}

inline Clipper2Lib::PathsD
generateSparseRectangleInfill(float density, const Clipper2Lib::PathsD &area,
                              float angle = 45.0f) {
  using namespace Clipper2Lib;
  auto [minX, minY, maxX, maxY] = GetBounds(area);
  double diagonal = distance({minX, minY}, {maxX, maxY});
  if (!angle)
    diagonal = angle;
  auto [rotatedMinX, rotatedMinY, rotatedMaxX, rotatedMaxY] = GetBounds(
      InflatePaths(area, diagonal, JoinType::Miter, EndType::Polygon));

  double step = 1.0f / density;
  bool leftToRight = true;

  Clipper2Lib::PathsD infill;
  for (double y = rotatedMinY; y <= rotatedMaxY; y += step) {
    if (leftToRight) {
      infill.push_back({
          {rotatedMinX, y},
          {rotatedMaxX, y},
      });
    } else {
      infill.push_back({
          {rotatedMaxX, y},
          {rotatedMinX, y},
      });
    }

    leftToRight = !leftToRight;
  }
  for (double x = rotatedMinX; x <= rotatedMaxX; x += step) {
    if (leftToRight) {
      infill.push_back({
          {x, rotatedMinY},
          {x, rotatedMaxY},
      });
    } else {
      infill.push_back({
          {x, rotatedMaxY},
          {x, rotatedMinY},
      });
    }

    leftToRight = !leftToRight;
  }

  if (angle != 0.0f)
    rotatePaths(infill, angle);

  ClipperD clipper;
  clipper.AddClip(area);
  clipper.AddOpenSubject(infill);
  PathsD discard, result;
  clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard, result);

  return result;
}

enum FillType {
  NoFill,
  Concentric,
  Lines,
  FillTypeCount,
};

enum AdhesionTypes {
  None,
  Brim,
  Skirt,
  Raft,
  Count,
};

enum BrimLocation {
  Outside,
  Inside,
  Both,
};

class Slicer {
public:
public:
  Slicer() = default;
  Slicer(const char *modelPath);
  void loadModel(const char *modelPath);
  Model &getModel() { return *m_model; };
  void init(float layerHeight, float nozzleDiameter);

  int getLayerCount() const { return m_layerCount; }
  bool hasSlices() const { return m_slices.size() > 0; }

  const Slice &getSlice(size_t index) const { return m_slices.at(index); }

  void createSlices();
  void createWalls(int wallCount);
  void createFill(FillType fillType, int floorCount, int roofCount);
  void createInfill(float density);
  void createSupport(float density);

  // Adhesion
  void createBrim(BrimLocation brimLocation, int lineCount);
  void createSkirt(int lineCount, int height, float distance);

  const char *fillTypes[FillTypeCount]{"None", "Concentric", "Lines"};

private:
  std::unique_ptr<Model> m_model;
  std::vector<Slice> m_slices;

  size_t m_layerCount = 0;
  float m_layerHeight;
  float m_lineWidth;
  int m_floorCount;
  int m_roofCount;
};