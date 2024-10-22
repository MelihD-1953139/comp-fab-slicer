#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "shader.h"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
};

class Mesh {
   public:
	Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices);

	void draw(Shader &shader, glm::vec4 color = glm::vec4(.3f, .3f, .3f, 1.0f));

	glm::vec3 getMin() const;
	glm::vec3 getMax() const;

   protected:
	void initialize();

   protected:
	std::vector<Vertex> m_vertices;
	std::vector<GLuint> m_indices;

	bool m_hasColor;
	unsigned int m_VAO, m_VBO, m_EBO;
};