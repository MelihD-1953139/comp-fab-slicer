#pragma once

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <glm/glm.hpp>

inline float distance(const glm::vec3 &a, const Clipper2Lib::PointD &b) {
  return glm::distance(a, glm::vec3(b.x, 0, b.y));
}

inline float distance(const Clipper2Lib::PointD &a, const glm::vec3 &b) {
  return glm::distance(glm::vec3(a.x, 0, a.y), b);
}

inline float distance(const Clipper2Lib::PointD &a,
                      const Clipper2Lib::PointD &b) {
  return glm::distance(glm::vec3(a.x, 0, a.y), glm::vec3(b.x, 0, b.y));
}