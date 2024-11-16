#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

#include "shader.h"
#include "slice.h"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
};

struct Triangle {
	std::array<glm::vec3, 3> vertices;

	Triangle(Vertex v1, Vertex v2, Vertex v3) : vertices({v1.position, v2.position, v3.position}) {}

	float getYmin() const { return std::min({vertices[0].y, vertices[1].y, vertices[2].y}); }
	float getYmax() const { return std::max({vertices[0].y, vertices[1].y, vertices[2].y}); }
};

class Mesh {
   public:
	Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices);

	void draw(Shader &shader, glm::vec3 color);

	glm::vec3 getMin() const;
	glm::vec3 getMax() const;

	const std::vector<Triangle> &getTriangles() const { return m_triangles; }

	std::optional<Slice> getSlice(double sliceHeight);

   protected:
	void initialize();

   protected:
	std::vector<Vertex> m_vertices;
	std::vector<GLuint> m_indices;
	std::vector<Triangle> m_triangles;

	bool m_hasColor;
	unsigned int m_VAO, m_VBO, m_EBO;
};