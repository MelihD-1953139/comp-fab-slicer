#include "slice.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <Nexus.h>

#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <set>

Contour::Contour(std::vector<glm::vec3> points) : m_points(points) {
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

Contour::Contour(Clipper2Lib::PathD path) {
	for (auto &point : path) {
		m_points.emplace_back(point.x, 0, point.y);
	}
	init();
}

void Contour::draw(Shader &shader, glm::vec3 color) {
	shader.use();
	shader.setVec3("color", color);

	glBindVertexArray(m_VAO);
	glDrawArrays(GL_LINE_STRIP, 0, m_points.size());
	glBindVertexArray(0);
}

Contour::operator Clipper2Lib::PathD() const {
	Clipper2Lib::PathD path;
	for (auto &point : m_points) {
		path.emplace_back(point.x, point.z);
	}
	return path;
}

Slice::Slice(std::vector<Line> lineSegments) {
	std::vector<glm::vec3> points;
	while (!lineSegments.empty()) {
		if (points.empty()) {
			points.push_back(lineSegments[0].p1);
			points.push_back(lineSegments[0].p2);
			lineSegments.erase(lineSegments.begin());
		}

		for (int i = 0; i < lineSegments.size(); ++i) {
			auto line = lineSegments[i];
			if (glm::distance(points.back(), line.p1) < EPSILON) {
				if (glm::distance(points.front(), line.p2) < EPSILON) {
					points.push_back(points.front());
					m_contours.emplace_back(points);
					points.clear();
					lineSegments.erase(lineSegments.begin() + i);
				} else {
					points.push_back(line.p2);
					lineSegments.erase(lineSegments.begin() + i);
				}

				break;
			} else if (glm::distance(points.back(), line.p2) < EPSILON) {
				if (glm::distance(points.front(), line.p1) < EPSILON) {
					points.push_back(points.front());
					m_contours.emplace_back(points);
					points.clear();
					lineSegments.erase(lineSegments.begin() + i);
				} else {
					points.push_back(line.p1);
					lineSegments.erase(lineSegments.begin() + i);
				}
			}
		}
	}
}

Slice::Slice(const Clipper2Lib::PathsD &paths) {
	for (auto &path : paths) {
		m_contours.emplace_back(path);
	}
}

void Slice::render(Shader &shader, const glm::mat4 view, const glm::mat4 &projection,
				   glm::vec3 color) {
	shader.setViewProjection(view, projection);
	for (auto &contour : m_contours) {
		contour.draw(shader, color);
	}
}

Slice::operator Clipper2Lib::PathsD() const {
	Clipper2Lib::PathsD paths;
	for (auto &contour : m_contours) {
		paths.emplace_back(contour);
	}
	return paths;
}