#pragma once

#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

#include "clipper2/clipper.core.h"
#include "glm/fwd.hpp"
#include "shader.h"

#define GREEN glm::vec3(0.0f, 1.0f, 0.0f)
#define YELLOW glm::vec3(1.0f, 1.0f, 0.0f)
#define BLUE glm::vec3(0.0f, 0.0f, 1.0f)
#define RED glm::vec3(1.0f, 0.0f, 0.0f)
#define ORANGE glm::vec3(1.0f, 0.65f, 0.0f)

struct Line {
  Clipper2Lib::PointD p1, p2;
  void setNextPoint(glm::vec3 point);
  bool operator==(const Line &other) const;

private:
  bool firstPointSet = false;
};

class Contour {
  using PathD = Clipper2Lib::PathD;
  using PointD = Clipper2Lib::PointD;

public:
  Contour(std::vector<glm::vec3> points);
  Contour(PathD path, bool isClosed = true);
  void draw(Shader &shader, glm::vec3 color) const;
  const std::vector<PointD> &getPoints() const { return m_points; }
  std::vector<PointD> getPoints() { return m_points; }

  operator PathD() const;

private:
  std::vector<PointD> m_points;

  unsigned int m_VAO, m_VBO;

private:
  void initOpenGLBuffers();
};

class Slice {
  using PathsD = Clipper2Lib::PathsD;

public:
  Slice(std::vector<Line> lineSegments);
  Slice(const PathsD &shells, const PathsD &infill);
  std::pair<glm::vec2, glm::vec2> getBounds() const;

  void render(Shader &shader, const glm::vec3 &position,
              const float &scale) const;

  operator PathsD() const;

  const std::vector<Contour> &getShells() const { return m_shells; }
  const std::vector<Contour> &getInfill() const { return m_infill; }

private:
  const float EPSILON = 1e-5;
  float currentEpsilon = EPSILON;
  std::vector<Contour> m_shells;
  std::vector<Contour> m_infill;
};