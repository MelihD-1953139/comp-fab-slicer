#pragma once

#include "slice.h"
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <cmath>
#include <glm/glm.hpp>

#define INT2MM(x) (static_cast<double>(x) / 1000.0)
#define MM2INT(x) (std::llrint((x) * 1000 + 0.5 * (((x) > 0) - ((x) < 0))))

inline float distance(const Clipper2Lib::PointD &a,
                      const Clipper2Lib::PointD &b) {
  return glm::distance(glm::vec2(a.x, a.y), glm::vec2(b.x, b.y));
  return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

template <typename T>
inline Clipper2Lib::Path<T> closePath(const Clipper2Lib::Path<T> &path) {
  auto ret = path;
  if (path.front() != path.back())
    ret.push_back(path.front());
  return ret;
}

template <typename T>
inline Clipper2Lib::Paths<T> closePaths(const Clipper2Lib::Paths<T> &paths) {
  Clipper2Lib::Paths<T> ret;
  for (auto &path : paths)
    ret.push_back(closePath(path));
  return ret;
}

inline void debugPrintLineSegments(const std::vector<Line> &lineSegments) {
  for (auto &line : lineSegments) {
    std::cout << "Line: " << line.p1.x << ", " << line.p1.y << " -> "
              << line.p2.x << ", " << line.p2.y << std::endl;
  }
}

template <typename T>
inline void debugPrintPath(const Clipper2Lib::Path<T> &path) {
  for (auto &point : path) {
    std::cout << "(" << point.x << ", " << point.y << ") ";
  }
  std::cout << std::endl;
}

template <typename T>
inline void debugPrintPaths(const Clipper2Lib::Paths<T> &paths) {
  for (auto &path : paths) {
    debugPrintPath(path);
  }
}

inline Clipper2Lib::Point64 toPoint64(const Clipper2Lib::PointD &point) {
  return {MM2INT(point.x), MM2INT(point.y)};
}

inline Clipper2Lib::Path64 toPath64(const Clipper2Lib::PathD &path) {
  Clipper2Lib::Path64 ret;
  for (auto &point : path) {
    ret.push_back(toPoint64(point));
  }
  return ret;
}

inline Clipper2Lib::Paths64 toPaths64(const Clipper2Lib::PathsD &paths) {
  Clipper2Lib::Paths64 ret;
  for (auto &path : paths) {
    ret.push_back(toPath64(path));
  }
  return ret;
}
inline Clipper2Lib::PointD toPointD(const Clipper2Lib::Point64 &point) {
  return {INT2MM(point.x), INT2MM(point.y)};
}

inline Clipper2Lib::PathD toPathD(const Clipper2Lib::Path64 &path) {
  Clipper2Lib::PathD ret;
  for (auto &point : path) {
    ret.push_back(toPointD(point));
  }
  return ret;
}

inline Clipper2Lib::PathsD toPathsD(const Clipper2Lib::Paths64 &paths) {
  Clipper2Lib::PathsD ret;
  for (auto &path : paths) {
    ret.push_back(toPathD(path));
  }
  return ret;
}
