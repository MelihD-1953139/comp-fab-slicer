#include "slice.h"

#include <Nexus.h>
#include <clipper2/clipper.h>
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

void Contour::draw(Shader &shader, glm::vec3 color) {
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

    for (int i = 0; i < lineSegments.size(); ++i) {
      auto line = lineSegments[i];
      if (glm::distance(points.back(), line.p1) < EPSILON) {
        if (glm::distance(firstPoint, line.p2) < EPSILON) {
          points.push_back(firstPoint);
          m_shells.emplace_back(points);
          points.clear();
          lineSegments.erase(lineSegments.begin() + i);
        } else {
          points.push_back(line.p2);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      } else if (glm::distance(points.back(), line.p2) < EPSILON) {
        if (glm::distance(firstPoint, line.p1) < EPSILON) {
          points.push_back(firstPoint);
          m_shells.emplace_back(points);
          points.clear();
          lineSegments.erase(lineSegments.begin() + i);
        } else {
          points.push_back(line.p1);
          lineSegments.erase(lineSegments.begin() + i);
        }
      }
    }
  }
}

Slice::Slice(const PathsD &paths, const PathsD &infill) {
  for (auto &path : paths) {
    m_shells.emplace_back(path);
  }
  for (auto &path : infill) {
    m_infill.emplace_back(path, false);
  }
}

void Slice::render(Shader &shader, const glm::mat4 view,
                   const glm::mat4 &projection) {
  shader.setMVP(glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(10.0f)),
                               glm::vec3(-100, 0, -100)),
                view, projection);
  for (auto &contour : m_shells) {
    contour.draw(shader, GREEN);
  }
  for (auto &contour : m_infill) {
    contour.draw(shader, BLUE);
  }
}

Slice::operator PathsD() const {
  PathsD paths;
  for (auto &contour : m_shells) {
    paths.emplace_back(contour);
  }
  return paths;
}
