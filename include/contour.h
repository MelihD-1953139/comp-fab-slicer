#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "shader.h"

struct Line {
	glm::vec3 p1, p2;

	void setNextPoint(glm::vec3 point) {
		if (firstPointSet) {
			p2 = point;
		} else {
			p1 = point;
			firstPointSet = true;
		}
	}

   private:
	bool firstPointSet = false;
};

class Contour {
   public:
	Contour() = default;
	Contour(std::vector<Line> lineSegments);
	void draw(Shader& shader, glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

   private:
	std::vector<glm::vec3> m_points;
	const float EPSILON = 0.001;

	unsigned int m_VAO, m_VBO;

   private:
	void initialize();
};