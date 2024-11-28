#include "printer.h"
#include "glm/gtc/type_ptr.hpp"

#include <Nexus.h>
#include <glm/gtc/matrix_transform.hpp>

Printer::Printer(const char *base, const char *slicePath, glm::ivec3 size,
                 float nozzle)
    : m_base(base), m_slicePlane(slicePath), m_size(size), m_nozzle(nozzle) {
  setSize(size);
}

void Printer::setSize(glm::ivec3 size) {
  m_size = size;
  m_base.setScale(glm::vec3(m_size));
  auto position = glm::vec3(m_size) * glm::vec3(0.5f, 0.0f, 0.5f);
  m_base.setPosition(position);
  m_slicePlane.setScale(glm::vec3(m_size) * glm::vec3(1.0f, 0.0f, 1.0f));
  m_slicePlane.setPosition(position);
}

glm::ivec3 Printer::getSize() const { return m_size; }
int *Printer::getSizePtr() { return glm::value_ptr(m_size); }
glm::vec3 Printer::getCenter() const { return m_base.getCenter(); }
float Printer::getNozzle() const { return m_nozzle; }
float *Printer::getNozzlePtr() { return &m_nozzle; }

void Printer::setSliceHeight(float sliceHeight) {
  auto pos = m_slicePlane.getPosition();
  m_slicePlane.setPosition({pos.x, sliceHeight, pos.z});
}

void Printer::render(Shader &shader, const glm::mat4 &view,
                     const glm::mat4 &projection, const glm::vec3 &colorBase,
                     const glm::vec3 &colorSlice, bool showSlicePlane) {
  m_base.render(shader, view, projection, colorBase);
  if (showSlicePlane)
    m_slicePlane.render(shader, view, projection, colorSlice);
}