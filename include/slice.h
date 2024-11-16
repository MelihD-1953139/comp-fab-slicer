#pragma once

#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
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

	void sort(glm::vec3 referencePoint) {
		double angle1 = glm::atan(-(p1.z - referencePoint.z), p1.x - referencePoint.x);
		double angle2 = glm::atan(-(p2.z - referencePoint.z), p2.x - referencePoint.x);

		bool crossingXAxis = (p1.z > referencePoint.z && p2.z < referencePoint.z) ||
							 (p1.z < referencePoint.z && p2.z > referencePoint.z);

		auto calculateXaxisCrossing = [](glm::vec3 p1, glm::vec3 p2, glm::vec3 referencePoint) {
			return p1.x + (referencePoint.z - p1.z) * (p2.x - p1.x) / (p2.z - p1.z);
		};

		if (crossingXAxis && calculateXaxisCrossing(p1, p2, referencePoint) < referencePoint.x) {
			std::swap(p1, p2);
			std::swap(angle1, angle2);
		} else if (angle2 > angle1) {
			std::swap(p1, p2);
			std::swap(angle1, angle2);
		}
	}

	Line operator*(glm::vec3 scale) {
		p1 *= scale;
		p2 *= scale;
		return *this;
	}

   private:
	bool firstPointSet = false;
};

class Contour {
   public:
	Contour(std::vector<glm::vec3> points);
	Contour(Clipper2Lib::PathD path);
	void draw(Shader& shader, glm::vec4 color);
	const std::vector<glm::vec3>& getPoints() const { return m_points; }

	operator Clipper2Lib::PathD() const;

   private:
	std::vector<glm::vec3> m_points;

	unsigned int m_VAO, m_VBO;

   private:
	void init();
};

class Slice {
   public:
	Slice(std::vector<Line> lineSegments);
	Slice(const Clipper2Lib::PathsD& paths);

	void render(Shader& shader, const glm::mat4 view, const glm::mat4& projection,
				glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	operator Clipper2Lib::PathsD() const;

   private:
	const float EPSILON = 0.001;
	std::vector<Contour> m_contours;
};