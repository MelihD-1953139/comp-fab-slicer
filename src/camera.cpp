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

// ==================== PerspectiveCamera ====================
PerspectiveCamera::PerspectiveCamera(float start_polar, float start_azimuth,
									 float distanceFromTarget)
	: Camera(distanceFromTarget) {
	m_polar_angle = start_polar;
	m_azimuthal_angle = start_azimuth;
}

const glm::mat4 PerspectiveCamera::getProjectionMatrix(int width, int height) const {
	return glm::perspective(glm::radians(m_fov), (float)width / (float)height, 0.1f, 1000.0f);
}

// ==================== OrthographicCamera ====================
OrthographicCamera::OrthographicCamera(float start_polar, float start_azimuth,
									   float distanceFromTarget)
	: Camera(distanceFromTarget) {
	m_polar_angle = start_polar;
	m_azimuthal_angle = start_azimuth;
}

const glm::mat4 OrthographicCamera::getProjectionMatrix(int width, int height) const {
	const float widthf = static_cast<float>(width);
	const float heightf = static_cast<float>(height);
	return glm::ortho(-widthf / 2, widthf / 2, -heightf / 2, heightf / 2, 0.1f, 1000.0f);
}