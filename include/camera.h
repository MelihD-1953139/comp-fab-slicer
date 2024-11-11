#pragma once

#include <glm/glm.hpp>

enum CameraMove { FORWARD, BACKWARD, LEFT, RIGHT };

// Defaults

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 5.f;
const float SENSITIVITY = 0.1f;
const float FOV_NORMAL = 45.0f;
const float FOV_ZOOM = 1.0f;
const float PITCH_MIN = -89.9f;
const float PITCH_MAX = 89.9f;
const glm::vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

class Camera {
   public:
	Camera(float distanceFromTarget = 5.0f);

	const glm::mat4 getViewMatrix(const glm::vec3 &target) const;
	const glm::mat4 getProjectionMatrix(int width, int height) const;
	void move(CameraMove direction, float deltaTime);
	void processLookingDirection(float xoff, float yoff);
	void setFov(float fov);
	glm::vec3 getPosition() const;
	void setDistanceFromTarget(float distance) { m_distanceFromTarget = distance; }
	void offsetDistanceFromTarget(float offset) { m_distanceFromTarget += offset; }
	const float getDistanceFromTarget() const { return m_distanceFromTarget; }

   private:
	glm::vec3 m_front;
	glm::vec3 m_up;
	glm::vec3 m_right;

	float m_yaw;
	float m_pitch;

	float m_movementSpeed;
	float m_mouseSensitivity;
	float m_fov;

	float m_distanceFromTarget;

   private:
	void updateCameraVectors();
};