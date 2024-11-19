#include "camera.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

// Public
Camera::Camera(float polarAngle, float azimuthalAngle, float distanceFromTarget)
    : m_distanceFromTarget(distanceFromTarget), m_polarAngle(polarAngle),
      m_azimuthalAngle(azimuthalAngle) {
  updateCameraVectors();
}

const glm::mat4 Camera::getViewMatrix(const glm::vec3 &target) const {
  return glm::lookAt(getPosition() + target, target, m_up);
}

void Camera::orbit(glm::vec2 offset) {
  offset = glm::normalize(offset);
  m_polarAngle += offset.x * m_speed;
  m_azimuthalAngle += offset.y * m_speed;
  updateCameraVectors();
}

glm::vec3 Camera::getPosition() const {
  auto r = m_distanceFromTarget * glm::sin(glm::radians(m_azimuthalAngle));
  auto x = r * glm::sin(glm::radians(m_polarAngle));
  auto y = m_distanceFromTarget * glm::cos(glm::radians(m_azimuthalAngle));
  auto z = r * glm::cos(glm::radians(m_polarAngle));
  return {x, y, z};
}

void Camera::updateCameraVectors() {
  m_up = glm::rotateX(glm::vec3(0.0f, 0.0f, -1.0f),
                      glm::radians(m_azimuthalAngle));
  m_up = glm::rotateY(m_up, glm::radians(m_polarAngle));
}

// ==================== PerspectiveCamera ====================
PerspectiveCamera::PerspectiveCamera(float polarAngle, float azimuthalAngle,
                                     float distanceFromTarget)
    : Camera(polarAngle, azimuthalAngle, distanceFromTarget) {}

const glm::mat4 PerspectiveCamera::getProjectionMatrix(int width,
                                                       int height) const {
  return glm::perspective(glm::radians(m_fov), (float)width / (float)height,
                          0.1f, 1000.0f);
}

// ==================== OrthographicCamera ====================
OrthographicCamera::OrthographicCamera(float polarAngle, float azimuthalAngle,
                                       float distanceFromTarget)
    : Camera(polarAngle, azimuthalAngle, distanceFromTarget) {}

const glm::mat4 OrthographicCamera::getProjectionMatrix(int width,
                                                        int height) const {
  const float widthf = static_cast<float>(width);
  const float heightf = static_cast<float>(height);
  return glm::ortho(-widthf / 2, widthf / 2, -heightf / 2, heightf / 2, 0.1f,
                    1000.0f);
}