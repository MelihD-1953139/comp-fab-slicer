#include "mesh.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "resourceManager.h"

//==================== Public ====================
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices)
	: m_vertices(vertices), m_indices(indices) {
	initialize();
	for (size_t i = 0; i < m_indices.size(); i += 3) {
		m_triangles.emplace_back(m_vertices[m_indices[i]], m_vertices[m_indices[i + 1]],
								 m_vertices[m_indices[i + 2]]);
	}
}

void Mesh::draw(Shader &shader, glm::vec3 color) {
	shader.use();
	shader.setVec3("color", color);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
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

std::optional<Slice> Mesh::getSlice(double sliceHeight) {
	std::vector<Line> lineSegments;
	for (auto &triangle : m_triangles) {
		if (triangle.getYmin() >= sliceHeight || triangle.getYmax() <= sliceHeight) continue;

		Line segment;

		for (int i = 0; i < 3; ++i) {
			auto &v1 = triangle.vertices[i];
			auto &v2 = triangle.vertices[(i + 1) % 3];
			if ((v1.y - sliceHeight) * (v2.y - sliceHeight) > 0) continue;

			float t = (sliceHeight - v1.y) / (v2.y - v1.y);
			segment.setNextPoint(v1 + t * (v2 - v1));
		}
		lineSegments.push_back(segment * glm::vec3(1.0f, 0.0f, 1.0f));
	}

	// auto referencePoint = glm::vec3(0.0f);
	// for (auto &line : lineSegments) {
	// 	line *= glm::vec3(1.0f, 0.0f, 1.0f);
	// 	referencePoint += line.p1 + 0.5f * (line.p2 - line.p1);
	// }
	// referencePoint /= lineSegments.size();

	// std::cout << "Reference point: " << referencePoint.x << " " << referencePoint.z << std::endl;

	// std::sort(lineSegments.begin(), lineSegments.end(),
	// 		  [&referencePoint](const Line &a, const Line &b) {
	// 			  auto centerA = a.p1 + 0.5f * (a.p2 - a.p1);
	// 			  auto centerB = b.p1 + 0.5f * (b.p2 - b.p1);

	// 			  return glm::atan(-(centerA.z - referencePoint.z), centerA.x - referencePoint.x) >
	// 					 glm::atan(-(centerB.z - referencePoint.z), centerB.x - referencePoint.x);
	// 		  });

	// // std::cout << "After sorting" << std::endl;
	// for (auto &line : lineSegments) {
	// 	line.sort(referencePoint);
	// 	auto center = line.p1 + 0.5f * (line.p2 - line.p1);
	// 	std::cout << line.p1.x << " " << line.p1.z << " -> " << line.p2.x << " " << line.p2.z
	// 			  << std::endl;
	// }

	if (!lineSegments.empty()) return {lineSegments};
	return std::nullopt;
}