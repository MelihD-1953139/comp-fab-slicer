#include "printer.h"
#include "resourceManager.h"

#include <Nexus.h>
#include <glm/gtc/matrix_transform.hpp>

Printer::Printer(const char *base, const char *slicePath, glm::ivec3 size,
                 float nozzle)
    : m_base(ResourceManager::loadModel("base", base)),
      m_slicePlane(ResourceManager::loadModel("slicePlane", slicePath)),
      m_size(size), m_nozzle(nozzle) {
  m_base.setScale(glm::vec3(m_size));
  m_slicePlane.setScale(glm::vec3(m_size) * glm::vec3(1.0f, 0.0f, 1.0f));
}

void Printer::setSize(glm::ivec3 size) {
  m_size = size;
  m_base.setScale(glm::vec3(m_size));
  m_slicePlane.setScale(glm::vec3(m_size) * glm::vec3(1.0f, 0.0f, 1.0f));
}

glm::ivec3 Printer::getSize() const { return m_size; }
glm::vec3 Printer::getCenter() const { return m_base.getCenter(); }
float Printer::getNozzle() const { return m_nozzle; }
float *Printer::getNozzlePtr() { return &m_nozzle; }

void Printer::setSliceHeight(float sliceHeight) {
  auto pos = m_slicePlane.getPosition();
  m_slicePlane.setPosition({pos.x, sliceHeight, pos.z});
}

void Printer::render(Shader &shader, const glm::mat4 &view,
                     const glm::mat4 &projection, const glm::vec3 &colorBase,
                     const glm::vec3 &colorSlice) {
  m_base.render(shader, view, projection, colorBase);
  m_slicePlane.render(shader, view, projection, colorSlice);
}