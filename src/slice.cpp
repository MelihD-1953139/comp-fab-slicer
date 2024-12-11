#include "slice.h"
#include "utils.h"

#include <Nexus.h>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <cstdlib>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sys/types.h>

using namespace Clipper2Lib;

void Line::setNextPoint(Clipper2Lib::PointD point) {
  if (firstPointSet) {
    p2 = point;
  } else {
    p1 = point;
    firstPointSet = true;
  }
}

bool Line::operator==(const Line &other) const {
  return (p1 == other.p1 && p2 == other.p2) ||
         (p1 == other.p2 && p2 == other.p1);
}

Slice::Slice(std::vector<Line> lineSegments) {
  PathsD perimeter;
  PathD path;
  while (!lineSegments.empty()) {
    if (path.empty()) {
      path.push_back(lineSegments[0].p1);
      path.push_back(lineSegments[0].p2);
      lineSegments.erase(lineSegments.begin());
    }

    auto &firstPoint = path.front();
    size_t currentPointsCount = path.size();

    for (int i = 0; i < lineSegments.size(); ++i) {
      auto line = lineSegments[i];
      if (distance(path.back(), line.p1) < currentEpsilon) {
        if (distance(firstPoint, line.p2) < currentEpsilon) {
          path.push_back(firstPoint);
          perimeter.emplace_back(path);
          path.clear();
          lineSegments.erase(lineSegments.begin() + i);
          currentEpsilon = EPSILON;
        } else {
          path.push_back(line.p2);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      } else if (distance(path.back(), line.p2) < currentEpsilon) {
        if (distance(firstPoint, line.p1) < currentEpsilon) {
          path.push_back(firstPoint);
          perimeter.emplace_back(SimplifyPath(path, currentEpsilon));
          path.clear();
          lineSegments.erase(lineSegments.begin() + i);
          currentEpsilon = EPSILON;
        } else {
          path.push_back(line.p1);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      }
    }
    if (currentPointsCount == path.size())
      currentEpsilon *= 10;
  }
  m_shells.push_back(perimeter);
}

Slice::Slice(const std::vector<PathsD> &shells,
             const std::vector<PathsD> &fill) {
  for (auto &paths : shells) {
    auto inserted = m_shells.emplace_back(closePathsD(paths));
    initOpenGLBuffer(inserted);
  }
  for (auto &paths : fill) {
    m_infill.emplace_back(paths);
    initOpenGLBuffer(paths);
  }
}

std::pair<glm::vec2, glm::vec2> Slice::getBounds() const {
  glm::vec2 min = {std::numeric_limits<double>::max(),
                   std::numeric_limits<double>::max()};
  glm::vec2 max = {-std::numeric_limits<double>::max(),
                   -std::numeric_limits<double>::max()};
  for (auto &path : m_shells.front()) {
    for (auto &point : path) {
      min.x = std::min(min.x, float(point.x));
      min.y = std::min(min.y, float(point.y));
      max.x = std::max(max.x, float(point.x));
      max.y = std::max(max.y, float(point.y));
    }
  }
  return {min, max};
}

void Slice::render(Shader &shader, const glm::vec3 &position,
                   const float &scale) const {
  auto [min, max] = getBounds();
  auto center = (min + max) / 2.0f;
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model,
                         position - scale * glm::vec3(center.x, 0, center.y));
  model = glm::scale(model, glm::vec3(scale));
  shader.setMat4("model", model);

  size_t vaoIndex = 0;

  drawPaths(m_shells.front(), shader, RED, vaoIndex);

  for (auto it = m_shells.rbegin(); it != m_shells.rend() - 1; ++it)
    drawPaths(*it, shader, GREEN, vaoIndex);

  for (auto &fill : m_infill)
    drawPaths(fill, shader, ORANGE, vaoIndex);
}

void Slice::initOpenGLBuffer(const Clipper2Lib::PathsD &paths) {
  for (auto path : paths) {
    initOpenGLBuffer(path);
  }
}

void Slice::initOpenGLBuffer(const Clipper2Lib::PathD &path) {
  uint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, path.size() * sizeof(PointD), path.data(),
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_DOUBLE, GL_FALSE, sizeof(PointD), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  m_VAOs.push_back(VAO);
}

void Slice::drawPaths(const PathsD &paths, Shader &shader, glm::vec3 color,
                      size_t &vaoIndex) const {
  shader.use();
  shader.setVec3("color", color);
  for (auto &path : paths)
    drawPath(path, vaoIndex);
}
void Slice::drawPath(const PathD &path, size_t &vaoIndex) const {
  glBindVertexArray(m_VAOs[vaoIndex++]);
  glDrawArrays(GL_LINE_STRIP, 0, path.size());
  glBindVertexArray(0);
}
