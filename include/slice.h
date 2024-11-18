#pragma once

#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

#include "shader.h"

struct Line {
  glm::vec3 p1, p2;
  void setNextPoint(glm::vec3 point);
  void sort(glm::vec3 referencePoint);
  Line operator*(glm::vec3 scale);

private:
  bool firstPointSet = false;
};

class Contour {
public:
  Contour(std::vector<glm::vec3> points);
  Contour(Clipper2Lib::PathD path);
  void draw(Shader &shader, glm::vec3 color);
  const std::vector<glm::vec3> &getPoints() const { return m_points; }

  operator Clipper2Lib::PathD() const;

private:
  std::vector<glm::vec3> m_points;

  unsigned int m_VAO, m_VBO;

private:
  void initOpenGLBuffers();
};

class Slice {
public:
  Slice(std::vector<Line> lineSegments);
  Slice(const Clipper2Lib::PathsD &paths);

  void render(Shader &shader, const glm::mat4 view, const glm::mat4 &projection,
              glm::vec3 color);

  operator Clipper2Lib::PathsD() const;

  const std::vector<Contour> &getContours() const { return m_contours; }

private:
  const float EPSILON = 0.001;
  std::vector<Contour> m_contours;
};