#pragma once

#include <glm/glm.hpp>

class Camera {
   public:
	Camera(float distanceFromTarget);

	const glm::mat4 getViewMatrix(const glm::vec3 &target) const;
	virtual const glm::mat4 getProjectionMatrix(int width, int height) const = 0;

	void orbit(float xoffset, float yoffset) { orbit(glm::vec2(xoffset, yoffset)); }
	void orbit(glm::vec2 offset);

	void offsetDistanceFromTarget(float offset) { m_distanceFromTarget += offset; }
	const float getDistanceFromTarget() const { return m_distanceFromTarget; }
	void setDistanceFromTarget(float distance) { m_distanceFromTarget = distance; }

	glm::vec3 getPosition() const;

   protected:
	void updateCameraVectors();
	float m_distanceFromTarget;

	float m_polar_angle = 0.0f;
	float m_azimuthal_angle = 45.0f;

	glm::vec3 m_up = {0.0f, 1.0f, 0.0f};

	const float m_speed = 5.f;
	const glm::vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
};

class PerspectiveCamera : public Camera {
   public:
	PerspectiveCamera(float start_polar, float start_azimuth, float distanceFromTarget);
	virtual const glm::mat4 getProjectionMatrix(int width, int height) const override;

   private:
	const float m_fov = 90.0f;
};

class OrthographicCamera : public Camera {
   public:
	OrthographicCamera(float start_polar, float start_azimuth, float distanceFromTarget);
	virtual const glm::mat4 getProjectionMatrix(int width, int height) const override;
};