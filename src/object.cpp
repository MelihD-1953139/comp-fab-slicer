#include "object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "resourceManager.h"

// ========================= Object =========================
Object::Object(Model &model, Shader &shader, glm::vec3 pos)
	: m_model(model), m_shader(shader), m_scale(1.0f), m_pos(pos) {}

void Object::render(glm::mat4 view, glm::mat4 projection, glm::vec3 color) {
	auto modelMatrix = glm::translate(glm::mat4(1.0f), m_pos);
	modelMatrix = glm::scale(modelMatrix, m_scale);

	m_shader.use();
	m_shader.setViewProjection(view, projection);
	m_shader.setMat4("model", modelMatrix);
	m_model.draw(m_shader, color);
}

void Object::scale(glm::vec3 scale) { m_scale = scale; }

void Object::scale(float scale) { m_scale = glm::vec3(scale); }

std::pair<glm::vec3, glm::vec3> Object::getBoundingBox() const {
	glm::vec4 scaledMin = glm::vec4(m_model.getMin(), 1.0f);
	glm::vec4 scaledMax = glm::vec4(m_model.getMax(), 1.0f);
	return {scaledMin / scaledMin.w, scaledMax / scaledMax.w};
}

glm::vec3 Object::getCenter() const {
	auto boundingBox = getBoundingBox();
	glm::vec3 center = (boundingBox.first + boundingBox.second) / 2.0f;
	return center * m_scale;
}

float Object::getHeightOfObject() const {
	auto boundingBox = getBoundingBox();
	float height = boundingBox.second.y - boundingBox.first.y;
	return height;
}

void Object::setPosition(glm::vec3 pos) { m_pos = pos; }

void Object::setPositionCentered(glm::vec3 pos) {
	glm::vec3 center = getCenter();
	center.y = 0.0f;
	pos -= center;
	setPosition(pos);
}

std::optional<Slice> Object::getSlice(double sliceHeight) { return m_model.getSlice(sliceHeight); }