#include "camera.h"

#include <glad/gl.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>

#include "Nexus/Log.h"

// Public
Camera::Camera(float distanceFromTarget) : m_distanceFromTarget(distanceFromTarget) {
	updateCameraVectors();
}

const glm::mat4 Camera::getViewMatrix(const glm::vec3 &target) const {
	return glm::lookAt(getPosition() + target, target, m_up);
}

const glm::mat4 Camera::getProjectionMatrix(int width, int height) const {
	return glm::perspective(glm::radians(m_fov), (float)width / (float)height, 0.1f, 1000.0f);
}

void Camera::orbit(glm::vec2 offset) {
	offset = glm::normalize(offset);
	m_polar_angle += offset.x * m_speed;
	m_azimuthal_angle += offset.y * m_speed;
	updateCameraVectors();
}

glm::vec3 Camera::getPosition() const {
	auto r = m_distanceFromTarget * glm::sin(glm::radians(m_azimuthal_angle));
	auto x = r * glm::sin(glm::radians(m_polar_angle));
	auto y = m_distanceFromTarget * glm::cos(glm::radians(m_azimuthal_angle));
	auto z = r * glm::cos(glm::radians(m_polar_angle));
	return {x, y, z};
}

void Camera::updateCameraVectors() {
	m_up = glm::rotateX(glm::vec3(0.0f, 0.0f, -1.0f), glm::radians(m_azimuthal_angle));
	m_up = glm::rotateY(m_up, glm::radians(m_polar_angle));
}