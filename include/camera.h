#pragma once

#include <glm/glm.hpp>

class Camera {
public:
  Camera(float polarAngle, float azimuthAngle, float distanceFromTarget);

  const glm::mat4 getViewMatrix(const glm::vec3 &target) const;
  virtual const glm::mat4 getProjectionMatrix(int width, int height) const = 0;

  void orbit(float xoffset, float yoffset) {
    orbit(glm::vec2(xoffset, yoffset));
  }
  void orbit(glm::vec2 offset);

  void offsetDistanceFromTarget(float offset) {
    m_distanceFromTarget += offset;
  }
  const float getDistanceFromTarget() const { return m_distanceFromTarget; }
  void setDistanceFromTarget(float distance) {
    m_distanceFromTarget = distance;
  }

  glm::vec3 getPosition() const;

protected:
  void updateCameraVectors();
  float m_distanceFromTarget;

  float m_polarAngle;
  float m_azimuthalAngle;

  glm::vec3 m_up = {0.0f, 1.0f, 0.0f};

  const float m_speed = 5.f;
  const glm::vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
};

class PerspectiveCamera : public Camera {
public:
  PerspectiveCamera(float polarAngle, float azimuthalAngle,
                    float distanceFromTarget);
  virtual const glm::mat4 getProjectionMatrix(int width,
                                              int height) const override;

private:
  const float m_fov = 90.0f;
};

class OrthographicCamera : public Camera {
public:
  OrthographicCamera(float polarAngle, float azimuthalAngle,
                     float distanceFromTarget);
  virtual const glm::mat4 getProjectionMatrix(int width,
                                              int height) const override;
};