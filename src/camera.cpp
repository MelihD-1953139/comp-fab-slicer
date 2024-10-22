#include "camera.h"

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Public
Camera::Camera() : fov(m_fov) {
	m_yaw = YAW;
	m_pitch = PITCH;
	m_fov = FOV_NORMAL;
	m_movementSpeed = SPEED;
	m_mouseSensitivity = SENSITIVITY;
}

Camera::Camera(glm::vec3 position) : Camera() { setStartPos(position); }

void Camera::setStartPos(glm::vec3 position) {
	m_position = position;
	m_startPosition = position;
}

const glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

const glm::mat4 Camera::getProjectionMatrix(int width, int height) const {
	return glm::perspective(glm::radians(m_fov), (float)width / (float)height, 0.1f, 100.0f);
}

void Camera::move(CameraMove direction, float deltaTime) {
	glm::vec3 front = m_front;
	glm::vec3 right = m_right;

	float vel = m_movementSpeed * deltaTime;
	if (direction == FORWARD)
		m_position += front * vel;
	else if (direction == BACKWARD)
		m_position -= front * vel;
	else if (direction == RIGHT)
		m_position += right * vel;
	else if (direction == LEFT)
		m_position -= right * vel;
}

void Camera::processLookingDirection(float xoff, float yoff) {
	xoff *= m_mouseSensitivity;
	yoff *= m_mouseSensitivity;

	m_yaw += xoff;
	m_pitch += yoff;

	if (m_pitch > PITCH_MAX)
		m_pitch = PITCH_MAX;
	else if (m_pitch < PITCH_MIN)
		m_pitch = PITCH_MIN;

	updateCameraVectors();
}

void Camera::ToggleFov() {
	if (m_fov == FOV_NORMAL)
		m_fov = FOV_ZOOM;
	else
		m_fov = FOV_NORMAL;
}

glm::vec3 Camera::getPosition() const { return m_position; }

// Private
void Camera::updateCameraVectors() {
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_front = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_front, WORLD_UP));
	m_up = glm::normalize(WORLD_UP);
}