#include "contour.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>

Contour::Contour(std::vector<Line> lineSegments) {
	const auto zeroY = glm::vec3(1.0f, 0.0f, 1.0f);
	m_points.push_back(lineSegments[0].p1 * zeroY);

	int pointCountNeeded = lineSegments.size();

	auto pointToMatch = lineSegments[0].p2 * zeroY;
	lineSegments.erase(lineSegments.begin());
	while (m_points.size() < pointCountNeeded) {
		for (int i = 0; i < lineSegments.size(); ++i) {
			auto line = lineSegments[i];
			if (glm::distance(pointToMatch, line.p1 * zeroY) < EPSILON) {
				m_points.push_back(line.p1 * zeroY + glm::vec3(0.0f, 1.0f, 0.0f));
				pointToMatch = line.p2 * zeroY;
				lineSegments.erase(lineSegments.begin() + i);
				break;
			}
			if (glm::distance(pointToMatch, line.p2 * zeroY) < EPSILON) {
				m_points.push_back(line.p2 * zeroY + glm::vec3(0.0f, 1.0f, 0.0f));
				pointToMatch = line.p1 * zeroY;
				lineSegments.erase(lineSegments.begin() + i);
				break;
			}
		}
	}
	initialize();
}

void Contour::draw(Shader &shader, glm::vec4 color) {
	shader.use();
	shader.setVec4("color", color);

	glBindVertexArray(m_VAO);
	glDrawArrays(GL_LINE_LOOP, 0, m_points.size());
	glBindVertexArray(0);
}

void Contour::initialize() {
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(glm::vec3), &m_points[0],
				 GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}