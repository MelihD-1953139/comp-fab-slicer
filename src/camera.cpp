#include "camera.h"

#include <glad/gl.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Public
Camera::Camera(float distanceFromTarget) {
	m_yaw = YAW;
	m_pitch = PITCH;
	m_fov = FOV_NORMAL;
	m_movementSpeed = SPEED;
	m_mouseSensitivity = SENSITIVITY;
	m_up = WORLD_UP;

	m_distanceFromTarget = distanceFromTarget;
}

void Camera::setFov(float fov) { m_fov = std::clamp(fov, 0.0f, 90.0f); }

const glm::mat4 Camera::getViewMatrix(const glm::vec3 &target) const {
	return glm::lookAt(m_front * m_distanceFromTarget, target, m_up);
}

const glm::mat4 Camera::getProjectionMatrix(int width, int height) const {
	return glm::perspective(glm::radians(m_fov), (float)width / (float)height, 0.1f, 1000.0f);
}

void Camera::move(CameraMove direction, float deltaTime) {
	float vel = m_movementSpeed * deltaTime;
	if (direction == FORWARD)
		m_pitch += vel;
	else if (direction == BACKWARD)
		m_pitch -= vel;
	else if (direction == RIGHT)
		m_yaw += vel;
	else if (direction == LEFT)
		m_yaw -= vel;

	m_pitch = std::clamp(m_pitch, PITCH_MIN, PITCH_MAX);
	updateCameraVectors();
}

void Camera::processLookingDirection(float xoff, float yoff) {
	xoff *= m_mouseSensitivity;
	yoff *= m_mouseSensitivity;

	m_yaw += xoff;
	m_pitch += yoff;

	m_pitch = std::clamp(m_pitch, 0.0f, 90.0f);

	updateCameraVectors();
}

glm::vec3 Camera::getPosition() const { return m_front * m_distanceFromTarget; }

// Private
void Camera::updateCameraVectors() {
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_front = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_front, m_up));
}