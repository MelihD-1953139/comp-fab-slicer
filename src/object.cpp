#include "object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "resourceManager.h"

// ========================= Object =========================
Object::Object(Model &model, Shader &shader, glm::vec3 pos, float scale)
	: m_model(model), m_shader(shader) {
	m_pos = pos;
	m_scale = scale;
	m_modelMatrix = glm::mat4(1.0f);
	m_modelMatrix = glm::translate(m_modelMatrix, pos);
}

void Object::render(glm::mat4 view, glm::mat4 projection, DirectionalLight dirLight,
					glm::vec4 color) {
	m_shader.use();
	m_shader.setViewProjection(view, projection);
	m_shader.setMat4("model", m_modelMatrix);
	setLighting(dirLight);

	m_model.draw(m_shader, color);
}

void Object::scale(glm::vec3 scale) { m_modelMatrix = glm::scale(m_modelMatrix, scale); }

void Object::scale(float scale) { m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(scale)); }

std::pair<glm::vec3, glm::vec3> Object::getBoundingBox() const {
	glm::vec4 scaledMin = m_modelMatrix * glm::vec4(m_model.getMin(), 1.0f);
	glm::vec4 scaledMax = m_modelMatrix * glm::vec4(m_model.getMax(), 1.0f);
	return {scaledMin / scaledMin.w, scaledMax / scaledMax.w};
}

glm::vec3 Object::getCenter() const {
	auto boundingBox = getBoundingBox();
	glm::vec3 center = (boundingBox.first + boundingBox.second) / 2.0f;
	return center;
}

void Object::addTransformation(glm::mat4 transformation) {
	m_modelMatrix = m_modelMatrix * transformation;
}

void Object::setLighting(DirectionalLight &dirLight) {
	m_shader.use();
	m_shader.setDirectionalLight(dirLight);
}

float Object::getHeightOfObject() const {
	auto boundingBox = getBoundingBox();
	float height = boundingBox.second.y - boundingBox.first.y;
	return height;
}