#pragma once

#include "slice.h"
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <cmath>
#include <glm/glm.hpp>

inline float distance(const Clipper2Lib::PointD &a,
                      const Clipper2Lib::PointD &b) {
  return glm::distance(glm::vec2(a.x, a.y), glm::vec2(b.x, b.y));
  return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

inline Clipper2Lib::PathD closePathD(const Clipper2Lib::PathD &path) {
  auto ret = path;
  if (path.front() != path.back())
    ret.push_back(path.front());
  return ret;
}

inline Clipper2Lib::PathsD closePathsD(const Clipper2Lib::PathsD &paths) {
  Clipper2Lib::PathsD ret;
  for (auto &path : paths)
    ret.push_back(closePathD(path));
  return ret;
}

inline void debugPrintLineSegments(const std::vector<Line> &lineSegments) {
  for (auto &line : lineSegments) {
    std::cout << "Line: " << line.p1.x << ", " << line.p1.y << " -> "
              << line.p2.x << ", " << line.p2.y << std::endl;
  }
}

inline void debugPrintPathD(const Clipper2Lib::PathD &path) {
  for (auto &point : path) {
    std::cout << "(" << point.x << ", " << point.y << ") ";
  }
  std::cout << std::endl;
}

inline void debugPrintPathsD(const Clipper2Lib::PathsD &paths) {
  for (auto &path : paths) {
    debugPrintPathD(path);
  }
}
