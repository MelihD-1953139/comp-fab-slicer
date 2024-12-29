#include "slice.h"
#include "Nexus/Log.h"
#include "utils.h"

#include <Nexus.h>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <cstdlib>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sys/types.h>
#include <vector>

using namespace Clipper2Lib;

void Line::setNextPoint(Clipper2Lib::PointD point) {
  // point = {std::round(point.x * 100.0) / 100.0,
  //          std::round(point.y * 100.0) / 100.0};
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

Slice::Slice(std::vector<Line> &lineSegments) {
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
      if (distance(path.back(), line.p1) < EPSILON) {
        if (distance(firstPoint, line.p2) < EPSILON) {
          path.push_back(firstPoint);
          perimeter.emplace_back(SimplifyPath(path, 0.1));
          path.clear();
          lineSegments.erase(lineSegments.begin() + i);
        } else {
          path.push_back(line.p2);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      } else if (distance(path.back(), line.p2) < EPSILON) {
        if (distance(firstPoint, line.p1) < EPSILON) {
          path.push_back(firstPoint);
          perimeter.emplace_back(SimplifyPath(path, 0.1));
          path.clear();
          lineSegments.erase(lineSegments.begin() + i);
        } else {
          path.push_back(line.p1);
          lineSegments.erase(lineSegments.begin() + i);
        }
        break;
      }
    }
  }
  perimeter = Clipper2Lib::Union(perimeter, FillRule::EvenOdd);
  m_paths.emplace(OuterWall, std::vector<PathsD>{perimeter});
}

void Slice::clear() {
  for (auto &pd : m_paths) {
    for (auto vao : pd.second.VAOs)
      glDeleteVertexArrays(1, &vao);
  }
  m_paths.clear();
  m_supportArea = PathsD();
}

void Slice::addOuterWall(const PathsD &wall) {
  if (!m_paths.contains(OuterWall))
    m_paths.emplace(OuterWall, PathData());

  auto &pd = m_paths.at(OuterWall);
  pd.paths.push_back(wall);
  initOpenGLBuffers(pd.VAOs, wall);
}

const PathsD &Slice::getPerimeter() const {
  return m_paths.at(OuterWall).paths.front();
}

const PathD Slice::getOuterMostPerimeter() const {
  auto perimeters = m_paths.at(OuterWall).paths;
  if (perimeters.empty()) {
    return PathD(); // Return an empty path if there are no perimeters
  }

  // for (auto &perimeter : perimeters) {
  //   Nexus::Logger::debug("Perimeter area {}", Area(perimeter));
  //   debugPrintPathsD(perimeter);
  // }

  // Initialize the outermost path with the first path
  PathD outermostPath = perimeters.front().front();
  double maxArea = Area(GetBounds(outermostPath).AsPath());

  // Loop through all PathsD and then through each PathD to find the one with
  // the largest area
  for (const auto &paths : perimeters) {
    for (const auto &path : paths) {
      double currentArea = Area(GetBounds(path).AsPath());
      if (currentArea > maxArea) {
        outermostPath = path;
        maxArea = currentArea;
      }
    }
  }

  return outermostPath;
}

void Slice::addInnerWall(const PathsD &shell) {
  if (!m_paths.contains(InnerWall))
    m_paths.emplace(InnerWall, PathData());

  auto &pd = m_paths.at(InnerWall);
  pd.paths.push_back(shell);
  initOpenGLBuffers(pd.VAOs, shell);
}

const std::vector<PathsD> &Slice::getShells() const {
  return m_paths.at(InnerWall).paths;
}
const PathsD &Slice::getInnermostShell() const {
  return m_paths.at(InnerWall).paths.back();
}

void Slice::addFill(const PathsD &fill) {
  if (!m_paths.contains(Skin))
    m_paths.emplace(Skin, PathData());

  auto &pd = m_paths.at(Skin);
  pd.paths.push_back(fill);
  initOpenGLBuffers(pd.VAOs, fill);
}

const std::vector<PathsD> &Slice::getFill() const {
  return m_paths.at(Skin).paths;
}

void Slice::addInfill(const PathsD &infill) {
  if (!m_paths.contains(Infill))
    m_paths.emplace(Infill, PathData());

  auto &pd = m_paths.at(Infill);
  pd.paths.push_back(infill);
  initOpenGLBuffers(pd.VAOs, infill);
}

const std::vector<PathsD> &Slice::getInfill() const {
  return m_paths.at(Infill).paths;
}

void Slice::addSupport(const PathsD &support) {
  if (!m_paths.contains(Support))
    m_paths.emplace(Support, PathData());

  auto &pd = m_paths.at(Support);
  pd.paths.push_back(support);
  initOpenGLBuffers(pd.VAOs, support);
}

const std::vector<PathsD> &Slice::getSupport() const {
  return m_paths.at(Support).paths;
}

void Slice::removeSupport() {
  if (!m_paths.contains(Support))
    return;
  auto VAOs = m_paths.at(Support).VAOs;
  glDeleteVertexArrays(VAOs.size(), VAOs.data());
  m_paths.at(Support).paths.clear();
  m_paths.at(Support).VAOs.clear();
}

std::pair<glm::vec2, glm::vec2> Slice::getBounds() const {
  auto [minX, minY, maxX, maxY] =
      GetBounds(m_paths.at(InnerWall).paths.front());
  return {{minX, minY}, {maxX, maxY}};
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

  drawPaths(m_paths.at(OuterWall), shader, RED);

  drawPaths(m_paths.at(InnerWall), shader, GREEN);

  drawPaths(m_paths.at(Skin), shader, YELLOW);

  drawPaths(m_paths.at(Infill), shader, ORANGE);

  drawPaths(m_paths.at(Support), shader, BLUE);
}

void Slice::initOpenGLBuffers(std::vector<uint> &VAOs, const PathsD &paths) {
  for (auto path : paths) {
    VAOs.push_back(initOpenGLBuffer(path));
  }
}

uint Slice::initOpenGLBuffer(const PathD &path) {
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
  return VAO;
}

void Slice::drawPaths(const PathData &pd, Shader &shader,
                      glm::vec3 color) const {
  shader.use();
  shader.setVec3("color", color);
  size_t vaoIndex = 0;
  for (auto &paths : pd.paths)
    for (auto &path : paths)
      drawPath(path, pd.VAOs[vaoIndex++]);
}

void Slice::drawPath(const PathD &path, uint vao) const {
  glBindVertexArray(vao);
  glDrawArrays(GL_LINE_STRIP, 0, path.size());
  glBindVertexArray(0);
}
