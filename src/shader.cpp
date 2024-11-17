#include "shader.h"

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

Shader::Shader() {
  m_id = glCreateProgram();
  m_toDeleteProgram = true;
}

Shader &Shader::operator=(Shader other) {
  m_id = other.m_id;
  m_hasVertShader = other.m_hasVertShader;
  m_hasFragShader = other.m_hasFragShader;
  m_isLinked = other.m_isLinked;
  m_vertShaderId = other.m_vertShaderId;
  m_fragShaderId = other.m_fragShaderId;
  other.m_toDeleteProgram = false;

  return *this;
}

Shader::~Shader() {
  if (m_toDeleteProgram)
    glDeleteProgram(m_id);
}

bool Shader::addVertexShader(const char *vertexData) {
  m_vertShaderId = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(m_vertShaderId, 1, &vertexData, nullptr);
  glCompileShader(m_vertShaderId);
  if (!checkShaderCompileError(m_vertShaderId, "VERTEX"))
    m_hasVertShader = false;
  m_hasVertShader = true;
  return m_hasVertShader;
}
bool Shader::addFragmentShader(const char *fragmentData) {
  m_fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(m_fragShaderId, 1, &fragmentData, nullptr);
  glCompileShader(m_fragShaderId);
  if (!checkShaderCompileError(m_fragShaderId, "FRAGMENT"))
    m_hasFragShader = false;
  m_hasFragShader = true;
  return m_hasFragShader;
}

bool Shader::linkProgram() {
  if (!m_hasVertShader || !m_hasFragShader) {
    std::cout << "ERROR::PROGRAM::LINKING_FAILED::NO_SHADERS_ADDED"
              << std::endl;
    return false;
  }
  if (m_hasVertShader)
    glAttachShader(m_id, m_vertShaderId);
  if (m_hasFragShader)
    glAttachShader(m_id, m_fragShaderId);
  glLinkProgram(m_id);
  if (!checkProgramLinkError())
    return false;

  glDeleteShader(m_vertShaderId);
  glDeleteShader(m_fragShaderId);
  m_isLinked = true;
  return true;
}

void Shader::use() { glUseProgram(m_id); }

void Shader::setBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
}
void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}
void Shader::setMat4(const std::string &name, glm::mat4 value) const {
  glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE,
                     glm::value_ptr(value));
}
void Shader::setVec3(const std::string &name, glm::vec3 value) const {
  glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1,
               glm::value_ptr(value));
}
void Shader::setVec4(const std::string &name, glm::vec4 value) const {
  glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1,
               glm::value_ptr(value));
}

void Shader::setMVP(const glm::mat4 &model, const glm::mat4 &view,
                    const glm::mat4 &projection) const {
  setMat4("model", model);
  setMat4("view", view);
  setMat4("projection", projection);
}

// Private functions
bool Shader::checkShaderCompileError(unsigned int shader, const char *type) {
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cout << "ERROR::SHADER::" << type << "::COMPILATION_FAILED" << infoLog
              << std::endl;
  }
  return success;
}
bool Shader::checkProgramLinkError() {
  int success;
  char infoLog[512];
  glGetProgramiv(m_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(m_id, 512, nullptr, infoLog);
    std::cout << "ERROR::PROGRAM" << m_id << "::LINKING_FAILED" << infoLog
              << std::endl;
  }
  return success;
}