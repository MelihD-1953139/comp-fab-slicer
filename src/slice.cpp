#include "slice.h"
#include "glm/fwd.hpp"

#include <Nexus.h>
#include <clipper2/clipper.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>

using namespace Clipper2Lib;

void Line::setNextPoint(glm::vec3 point) {
  if (firstPointSet) {
    p2 = point;
  } else {
    p1 = point;
    firstPointSet = true;
  }
}

Line Line::operator*(glm::vec3 scale) {
  p1 *= scale;
  p2 *= scale;
  return *this;
}

bool Line::operator==(const Line &other) const {
  return (p1 == other.p1 && p2 == other.p2) ||
         (p1 == other.p2 && p2 == other.p1);
}

Contour::Contour(std::vector<glm::vec3> points) : m_points(points) {
  initOpenGLBuffers();
}

Contour::Contour(PathD path, bool isClosed) {
  for (auto &point : path) {
    m_points.emplace_back(point.x, 0, point.y);
  }
  if (isClosed)
    m_points.push_back(m_points.front());
  initOpenGLBuffers();
}

void Contour::draw(Shader &shader, glm::vec3 color) const {
  shader.use();
  shader.setVec3("color", color);

  glBindVertexArray(m_VAO);
  glDrawArrays(GL_LINE_STRIP, 0, m_points.size());
  glBindVertexArray(0);
}

Contour::operator PathD() const {
  PathD path;
  for (auto &point : m_points) {
    path.emplace_back(point.x, point.z);
  }
  return path;
}

void Contour::initOpenGLBuffers() {
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(glm::vec3),
               &m_points[0], GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

Slice::Slice(std::vector<Line> lineSegments) {
  std::vector<glm::vec3> points;
  while (!lineSegments.empty()) {
    if (points.empty()) {
      points.push_back(lineSegments[0].p1);
      points.push_back(lineSegments[0].p2);
      lineSegments.erase(lineSegments.begin());
    }

    auto &firstPoint = points.front();
    size_t currentPointsCount = points.size();

    for (int i = 0; i < lineSegments.size(); ++i) {
      auto line = lineSegments[i];
      if (glm::distance(points.back(), line.p1) < currentEpsilon) {
        if (glm::distance(firstPoint, line.p2) < currentEpsilon) {
          points.push_back(firstPoint);
          m_shells.emplace_back(points);
          points.clear();
          lineSegments.erase(lineSegments.begin() + i);
          currentEpsilon = EPSILON;
        } else {
          points.push_back(line.p2);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      } else if (glm::distance(points.back(), line.p2) < currentEpsilon) {
        if (glm::distance(firstPoint, line.p1) < currentEpsilon) {
          points.push_back(firstPoint);
          m_shells.emplace_back(points);
          points.clear();
          lineSegments.erase(lineSegments.begin() + i);
          currentEpsilon = EPSILON;
        } else {
          points.push_back(line.p1);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      }
    }
    if (currentPointsCount == points.size())
      currentEpsilon *= 10;
  }
}

Slice::Slice(const PathsD &shells, const PathsD &infill,
             const PathsD &perimeters) {
  for (auto &path : shells) {
    m_shells.emplace_back(path);
  }
  for (auto &path : perimeters) {
    m_perimeters.emplace_back(path);
  }
  for (auto &path : infill) {
    m_infill.emplace_back(path, false);
  }
}

std::pair<glm::vec2, glm::vec2> Slice::getBounds() const {
  glm::vec2 min = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
  glm::vec2 max = {-std::numeric_limits<float>::max(),
                   -std::numeric_limits<float>::max()};
  for (auto &contour : m_perimeters) {
    for (auto &point : contour.getPoints()) {
      min.x = std::min(min.x, point.x);
      min.y = std::min(min.y, point.z);
      max.x = std::max(max.x, point.x);
      max.y = std::max(max.y, point.z);
    }
  }
  return {min, max};
}

void Slice::render(Shader &shader, const glm::vec3 &position,
                   const float &scale, const glm::mat4 view,
                   const glm::mat4 &projection) const {
  auto [min, max] = getBounds();
  auto center = (min + max) / 2.0f;
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model,
                         position - scale * glm::vec3(center.x, 0, center.y));
  model = glm::scale(model, glm::vec3(scale));
  shader.setMVP(model, view, projection);

  for (auto &contour : m_perimeters)
    contour.draw(shader, RED);
  for (auto &contour : m_shells)
    contour.draw(shader, GREEN);

  for (auto &contour : m_infill)
    contour.draw(shader, ORANGE);
}

Slice::operator PathsD() const {
  PathsD paths;
  for (auto &contour : m_shells) {
    paths.emplace_back(contour);
  }
  return SimplifyPaths(paths, 0.000001);
}
