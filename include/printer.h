#pragma once
#include <glm/glm.hpp>

#include "glm/fwd.hpp"
#include "model.h"

class Printer {
public:
  Printer(glm::ivec3 size = {235, 250, 235}, float nozzle = 0.4f);

  void setSize(glm::ivec3 size);
  glm::ivec3 getSize() const;
  int *getSizePtr();
  glm::vec3 getCenter() const;
  float getNozzle() const;
  float *getNozzlePtr();
  void setSliceHeight(float sliceHeight);

  void render(Shader &shader, const glm::mat4 &view,
              const glm::mat4 &projection, const glm::vec3 &colorBase,
              const glm::vec3 &colorSlice, bool showSlicePlane);

private:
  Model m_base;
  Model m_slicePlane;

  float m_nozzle;
  glm::ivec3 m_size;
  float m_sliceHeight;
};