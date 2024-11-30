#pragma once

#include <assimp/scene.h>

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

#include "shader.h"
#include "slice.h"

struct Triangle {
  std::array<glm::vec3, 3> vertices;

  Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3);

  float getYmin() const;
  float getYmax() const;

  glm::vec3 operator[](int i) const;
};

class Model {
public:
  Model(const char *path);
  Model(const char *data, size_t length);

  void render(Shader &shader, const glm::mat4 &view,
              const glm::mat4 &projection, const glm::vec3 &color);

  glm::vec3 getMin() const;
  glm::vec3 getMax() const;
  glm::vec3 getCenter() const;

  void setPosition(glm::vec3 position);
  glm::vec3 getPosition() const;
  float *getPositionPtr();
  void setRotation(glm::vec3 rotation);
  const glm::vec3 &getRotation() const;
  float *getRotationPtr();
  void setScale(glm::vec3 scale);
  const glm::vec3 &getScale() const;
  float *getScalePtr();

  float getHeight();

  Slice getSlice(double sliceHeight);

private:
  std::vector<glm::vec3> m_vertices;
  std::vector<GLuint> m_indices;
  std::vector<Triangle> m_triangles;

  glm::vec3 m_min;
  glm::vec3 m_max;
  glm::vec3 m_center;
  glm::vec3 m_position;
  glm::vec3 m_rotation;
  glm::vec3 m_scale;

  bool m_hasColor;
  unsigned int m_VAO, m_VBO, m_EBO;

private:
  void initOpenGLBuffers();
  void processVertices(const aiMesh *mesh);
  void processIndices(const aiMesh *mesh);
  void processTriangles();
  Triangle transformTriangle(const Triangle &triangle) const;
  glm::mat4 getModelMatrix() const;
};