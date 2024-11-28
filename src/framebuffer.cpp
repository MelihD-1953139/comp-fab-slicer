#include "framebuffer.h"

#include <Nexus/Log.h>
#include <glad/gl.h>

Framebuffer::Framebuffer(int width, int height) {
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  glEnable(GL_DEPTH_TEST);

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_texture, 0);

  glGenRenderbuffers(1, &m_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, m_rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Nexus::Logger::error("Framebuffer is not complete!");
  }

  clear();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
  glDeleteFramebuffers(1, &m_fbo);
  glDeleteTextures(1, &m_texture);
  glDeleteRenderbuffers(1, &m_rbo);
}

void Framebuffer::bind() { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }

void Framebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void Framebuffer::clear() {
  glClearColor(0.98, 0.98, 0.98, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::resize(int width, int height) {
  glViewport(0, 0, width, height);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);

  glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}