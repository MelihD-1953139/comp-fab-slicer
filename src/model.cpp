#include "model.h"
#include "Nexus/Log.h"
#include "glm/gtc/type_ptr.hpp"

#include <Nexus.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstddef>
#include <fstream>

#include <assimp/Importer.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>

// ======================= Triangle ========================

Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
    : vertices({v1, v2, v3}) {}
float Triangle::getYmin() const {
  return std::min({vertices[0].y, vertices[1].y, vertices[2].y});
}
float Triangle::getYmax() const {
  return std::max({vertices[0].y, vertices[1].y, vertices[2].y});
}
glm::vec3 Triangle::operator[](int i) const { return vertices[i]; }

// ========================= Model =========================
std::string readFile(const char *filePath) {
  std::string content;
  std::ifstream filestream(filePath, std::ios::in);

  if (!filestream.is_open()) {
    std::cout << "Could not read file " << filePath << ". File does not exist."
              << std::endl;
    return "";
  }

  std::string line = "";
  while (!filestream.eof()) {
    std::getline(filestream, line);
    content.append(line + "\n");
  }

  filestream.close();
  return content;
}

Model::Model(const char *path) : m_scale(1.0f) {
  using namespace Nexus;

  Assimp::Importer import;
  const aiScene *scene =
      import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    Logger::error("ASSIMP::{}", import.GetErrorString());

  if (scene->mRootNode->mNumMeshes > 1)
    Logger::error("Only one mesh per model is supported, {} provided. Only the "
                  "first mesh will be loaded",
                  scene->mRootNode->mNumMeshes);
  if (scene->mRootNode->mNumChildren > 1)
    Logger::error("Nested meshes is not supported");

  const auto mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[0]];

  processVertices(mesh);
  processIndices(mesh);
  processTriangles();

  initOpenGLBuffers();
}

Model::Model(const char *data, size_t length) : m_scale(1.0f) {
  using namespace Nexus;

  Assimp::Importer import;
  const aiScene *scene = import.ReadFileFromMemory(
      data, length, aiProcess_Triangulate | aiProcess_FlipUVs);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    Logger::error("ASSIMP::{}", import.GetErrorString());

  if (scene->mRootNode->mNumMeshes > 1)
    Logger::error("Only one mesh per model is supported, {} provided. Only the "
                  "first mesh will be loaded",
                  scene->mRootNode->mNumMeshes);
  if (scene->mRootNode->mNumChildren > 1)
    Logger::error("Nested meshes is not supported");

  const auto mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[0]];

  processVertices(mesh);
  processIndices(mesh);
  processTriangles();

  initOpenGLBuffers();
}

void Model::render(Shader &shader, const glm::mat4 &view,
                   const glm::mat4 &projection, const glm::vec3 &color) {
  shader.use();
  shader.setMVP(getModelMatrix(), view, projection);
  shader.setVec3("color", color);

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

glm::vec3 Model::getMin() const { return m_min * m_scale; }
glm::vec3 Model::getMax() const { return m_max * m_scale; }
glm::vec3 Model::getCenter() const { return m_center * m_scale; }
float Model::getHeight() {
  auto model = getModelMatrix();
  m_max = glm::vec3(-std::numeric_limits<float>::max());
  m_min = glm::vec3(std::numeric_limits<float>::max());

  for (auto &vertex : m_vertices) {
    auto transformed = model * glm::vec4(vertex.position, 1.0f);
    m_max = glm::max(m_max, glm::vec3(transformed));
    m_min = glm::min(m_min, glm::vec3(transformed));
  }

  return m_max.y - m_min.y;
}

void Model::setPosition(glm::vec3 position) { m_position = position; }
glm::vec3 Model::getPosition() const { return m_position; }
float *Model::getPositionPtr() { return glm::value_ptr(m_position); }

void Model::setRotation(glm::vec3 rotation) { m_rotation = rotation; }
const glm::vec3 &Model::getRotation() const { return m_rotation; }
float *Model::getRotationPtr() { return glm::value_ptr(m_rotation); }

void Model::setScale(glm::vec3 scale) { m_scale = scale; }
const glm::vec3 &Model::getScale() const { return m_scale; }
float *Model::getScalePtr() { return glm::value_ptr(m_scale); }

Slice Model::getSlice(double sliceHeight) {
  sliceHeight += +0.000000001;
  std::vector<Line> lineSegments;
  for (const auto &triangleOrig : m_triangles) {
    auto triangle = transformTriangle(triangleOrig);
    if (triangle.getYmin() >= sliceHeight || triangle.getYmax() <= sliceHeight)
      continue;

    Line segment;

    for (int i = 0; i < 3; ++i) {
      const auto &v1 = triangle[i];
      const auto &v2 = triangle[(i + 1) % 3];
      if ((v1.y - sliceHeight) * (v2.y - sliceHeight) > 0)
        continue;

      float t = (sliceHeight - v1.y) / (v2.y - v1.y);
      segment.setNextPoint((v1 + t * (v2 - v1)));
    }
    if (segment.p1 != segment.p2)
      lineSegments.push_back(segment * glm::vec3(1.0f, 0.0f, 1.0f));
  }

  return {lineSegments};
}

void Model::initOpenGLBuffers() {
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex),
               m_vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint),
               m_indices.data(), GL_STATIC_DRAW);

  // Pos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(0);
  // Normal
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void Model::processVertices(const aiMesh *mesh) {
  m_max = glm::vec3(-std::numeric_limits<float>::max());
  m_min = glm::vec3(std::numeric_limits<float>::max());

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
    glm::vec3 vector;
    vector.x = mesh->mVertices[i].x;
    vector.y = mesh->mVertices[i].z;
    vector.z = mesh->mVertices[i].y;

    glm::vec3 normal;
    if (mesh->mNormals) {
      normal.x = mesh->mNormals[i].x;
      normal.y = mesh->mNormals[i].z;
      normal.z = mesh->mNormals[i].y;
    }

    m_max = glm::max(m_max, vector);
    m_min = glm::min(m_min, vector);
    m_vertices.push_back(Vertex(vector, normal));
  }

  m_center = (m_max + m_min) / 2.0f;
  for (auto &vertex : m_vertices)
    vertex.position -= m_center;
}

void Model::processIndices(const aiMesh *mesh) {
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; ++j) {
      m_indices.push_back(face.mIndices[j]);
    }
  }
}

void Model::processTriangles() {
  for (size_t i = 0; i < m_indices.size(); i += 3) {
    m_triangles.emplace_back(m_vertices[m_indices[i]].position,
                             m_vertices[m_indices[i + 1]].position,
                             m_vertices[m_indices[i + 2]].position);
  }
}

glm::mat4 Model::getModelMatrix() const {
  glm::mat4 model(1.0f);
  model = glm::translate(model, m_position);
  model = glm::rotate(model, glm::radians(m_rotation.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(m_rotation.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(m_rotation.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, m_scale);
  return model;
}

Triangle Model::transformTriangle(const Triangle &triangle) const {
  glm::mat4 transformation = getModelMatrix();
  Triangle result(transformation * glm::vec4(triangle.vertices[0], 1.0f),
                  transformation * glm::vec4(triangle.vertices[1], 1.0f),
                  transformation * glm::vec4(triangle.vertices[2], 1.0f));
  return result;
}
