#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <vector>

#include "shader.h"
#include "slice.h"

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct Triangle {
  std::array<glm::vec3, 3> vertices;

  Triangle(Vertex v1, Vertex v2, Vertex v3);

  float getYmin() const;
  float getYmax() const;

  glm::vec3 operator[](int i) const;
};

class Model {
public:
  Model(const char *path);

  void render(Shader &shader, const glm::mat4 &view,
              const glm::mat4 &projection, const glm::vec3 &color);

  glm::vec3 getMin() const;
  glm::vec3 getMax() const;
  glm::vec3 getCenter() const;
  void setPosition(glm::vec3 position);
  void setPositionCentered(glm::vec3 centerPosition);
  glm::vec3 getPosition() const;
  void setScale(glm::vec3 scale);
  float getHeight() const;

  Slice getSlice(double sliceHeight);

private:
  std::vector<Vertex> m_vertices;
  std::vector<GLuint> m_indices;
  std::vector<Triangle> m_triangles;

  glm::vec3 m_min;
  glm::vec3 m_max;
  glm::vec3 m_center;
  glm::vec3 m_position;
  glm::vec3 m_scale;

  bool m_hasColor;
  unsigned int m_VAO, m_VBO, m_EBO;

private:
  void initOpenGLBuffers();
  void processVertices(const aiMesh *mesh);
  void processIndices(const aiMesh *mesh);
  void processTriangles();
};