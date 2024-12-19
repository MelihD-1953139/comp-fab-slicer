#pragma once

#include "glm/glm.hpp"
class Framebuffer {
public:
  Framebuffer(int width, int height);
  ~Framebuffer();

  void bind();
  void unbind();
  void clear(const glm::vec4 &color = glm::vec4(0.98f, 0.98f, 0.98f, 1.0f));

  void resize(int width, int height);

  unsigned int getTexture() const { return m_texture; }

private:
  unsigned int m_fbo;
  unsigned int m_texture;
  unsigned int m_rbo;
};