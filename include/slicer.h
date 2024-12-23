#pragma once
#include "Nexus/Log.h"
#include "glm/ext/matrix_transform.hpp"
#include "model.h"
#include "slice.h"
#include "state.h"
#include "utils.h"

#include <memory>
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
                       const Clipper2Lib::PathsD &lastShell) {
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
generateSparseRectangleInfill(float density, Clipper2Lib::RectD bounds,
                              float angle = 45.0f) {
  double minX = bounds.left;
  double maxX = bounds.right;
  double minY = bounds.top;
  double maxY = bounds.bottom;

  double step = 1.0f / density;
  bool leftToRight = true;

  Clipper2Lib::PathsD infill;
  for (double y = minY; y <= maxY; y += step) {
    if (leftToRight) {
      infill.push_back({
          {minX, y},
          {maxX, y},
      });
    } else {
      infill.push_back({
          {maxX, y},
          {minX, y},
      });
    }

    leftToRight = !leftToRight;
  }
  for (double x = minX; x <= maxX; x += step) {
    if (leftToRight) {
      infill.push_back({
          {x, minY},
          {x, maxY},
      });
    } else {
      infill.push_back({
          {x, maxY},
          {x, minY},
      });
    }

    leftToRight = !leftToRight;
  }

  if (angle != 0.0f)
    rotatePaths(infill, angle);
  return infill;
}

inline Clipper2Lib::PathsD
generateSparseRectangleInfill(float density, Clipper2Lib::PointD min,
                              Clipper2Lib::PointD max, float angle = 45.0f) {
  return generateSparseRectangleInfill(density, {min.x, min.y, max.x, max.y},
                                       angle);
}

class Slicer {
public:
  Slicer() = default;
  Slicer(const char *modelPath);
  void loadModel(const char *modelPath);
  Model &getModel() { return *m_model; };
  void init(float layerCount);

  int getLayerCount() { return m_layerCount; }
  bool hasSlices() { return m_slices.size() > 0; }

  const Slice &getSlice(size_t index) const { return m_slices.at(index); }

  void createSlices(float layerheight);
  void createWalls(int wallCount, float nozzleDiameter);
  void createFillAndInfill(float nozzleDiameter, int floorCount, int roofCount,
                           float infillDensity, const glm::ivec2 &printerSize);
  void createSupport(float nozzleDiameter, float density);

  // Adhesion
  void createBrim(int lineCount, float lineWidth);

private:
  std::unique_ptr<Model> m_model;
  std::vector<Slice> m_slices;

  size_t m_layerCount = 0;
};