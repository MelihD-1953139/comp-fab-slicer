#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <optional>
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

	void draw(Shader &shader, glm::vec3 color);

	glm::vec3 getMin() const;
	glm::vec3 getMax() const;

	Slice getSlice(double sliceHeight);

   private:
	std::vector<Vertex> m_vertices;
	std::vector<GLuint> m_indices;
	std::vector<Triangle> m_triangles;

	bool m_hasColor;
	unsigned int m_VAO, m_VBO, m_EBO;

	void initOpenGLBuffers();
	void processVertices(const aiMesh *mesh);
	void processIndices(const aiMesh *mesh);
	void processTriangles();
};