#pragma once

#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <sys/types.h>
#include <vector>

#include "clipper2/clipper.core.h"
#include "glm/fwd.hpp"
#include "shader.h"

#define GREEN glm::vec3(0.0f, 1.0f, 0.0f)
#define YELLOW glm::vec3(1.0f, 1.0f, 0.0f)
#define BLUE glm::vec3(0.0f, 0.0f, 1.0f)
#define RED glm::vec3(1.0f, 0.0f, 0.0f)
#define ORANGE glm::vec3(1.0f, 0.5f, 0.0f)

struct Line {
  Clipper2Lib::PointD p1, p2;
  void setNextPoint(Clipper2Lib::PointD point);
  bool operator==(const Line &other) const;

private:
  bool firstPointSet = false;
};

class Slice {
  using PathsD = Clipper2Lib::PathsD;

public:
  Slice(std::vector<Line> lineSegments);
  Slice(const std::vector<PathsD> &shells, const std::vector<PathsD> &infill);
  std::pair<glm::vec2, glm::vec2> getBounds() const;

  void render(Shader &shader, const glm::vec3 &position,
              const float &scale) const;

  const std::vector<PathsD> &getShells() const { return m_shells; }
  const std::vector<PathsD> &getInfill() const { return m_infill; }

private:
  const double EPSILON = 1e-5;
  double currentEpsilon = EPSILON;
  std::vector<PathsD> m_shells;
  std::vector<PathsD> m_infill;

  std::vector<uint> m_VAOs;

private:
  void initOpenGLBuffer(const Clipper2Lib::PathsD &paths);
  void initOpenGLBuffer(const Clipper2Lib::PathD &path);
  void drawPaths(const Clipper2Lib::PathsD &paths, Shader &shader,
                 glm::vec3 color, size_t &vaoIndex) const;
  void drawPath(const Clipper2Lib::PathD &path, size_t &vaoIndex) const;
};