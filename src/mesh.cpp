#include "mesh.h"

#include <glm/gtc/matrix_transform.hpp>

#include "resourceManager.h"
//==================== Public ====================
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices)
	: m_vertices(vertices), m_indices(indices) {
	initialize();
}

void Mesh::draw(Shader &shader, glm::vec4 color) {
	shader.use();
	shader.setVec4("color", color);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glActiveTexture(GL_TEXTURE0);
}

glm::vec3 Mesh::getMin() const {
	glm::vec3 min = m_vertices[0].position;
	for (unsigned int i = 1; i < m_vertices.size(); ++i) {
		if (m_vertices[i].position.x < min.x) min.x = m_vertices[i].position.x;
		if (m_vertices[i].position.y < min.y) min.y = m_vertices[i].position.y;
		if (m_vertices[i].position.z < min.z) min.z = m_vertices[i].position.z;
	}
	return min;
}

glm::vec3 Mesh::getMax() const {
	glm::vec3 max = m_vertices[0].position;
	for (unsigned int i = 1; i < m_vertices.size(); ++i) {
		if (m_vertices[i].position.x > max.x) max.x = m_vertices[i].position.x;
		if (m_vertices[i].position.y > max.y) max.y = m_vertices[i].position.y;
		if (m_vertices[i].position.z > max.z) max.z = m_vertices[i].position.z;
	}
	return max;
}

//==================== Private ====================
void Mesh::initialize() {
	// GLuint VAO;
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_EBO);

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0],
				 GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint), &m_indices[0],
				 GL_STATIC_DRAW);

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