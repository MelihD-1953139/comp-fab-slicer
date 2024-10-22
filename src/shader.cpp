#include "shader.h"

#include <glad/gl.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

Shader::Shader() {
	m_id = glCreateProgram();
	m_toDeleteProgram = true;
}

Shader &Shader::operator=(Shader other) {
	// TODO: I shoud not need this
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
	if (m_toDeleteProgram) glDeleteProgram(m_id);
}

bool Shader::addVertexShader(const char *vertexData) {
	m_vertShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertShaderId, 1, &vertexData, nullptr);
	glCompileShader(m_vertShaderId);
	if (!checkShaderCompileError(m_vertShaderId, "VERTEX")) m_hasVertShader = false;
	m_hasVertShader = true;
	return m_hasVertShader;
}
bool Shader::addFragmentShader(const char *fragmentData) {
	m_fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_fragShaderId, 1, &fragmentData, nullptr);
	glCompileShader(m_fragShaderId);
	if (!checkShaderCompileError(m_fragShaderId, "FRAGMENT")) m_hasFragShader = false;
	m_hasFragShader = true;
	return m_hasFragShader;
}

bool Shader::linkProgram() {
	if (!m_hasVertShader || !m_hasFragShader) {
		std::cout << "ERROR::PROGRAM::LINKING_FAILED::NO_SHADERS_ADDED" << std::endl;
		return false;
	}
	if (m_hasVertShader) glAttachShader(m_id, m_vertShaderId);
	if (m_hasFragShader) glAttachShader(m_id, m_fragShaderId);
	glLinkProgram(m_id);
	if (!checkProgramLinkError()) return false;

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
	glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setVec4(const std::string &name, glm::vec4 value) const {
	glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setMaterial(glm::vec3 &ambient, glm::vec3 &diffuse, glm::vec3 &specular,
						 glm::vec3 &emissive, float shininess) const {
	setVec3("material.ambient", ambient);
	setVec3("material.diffuse", diffuse);
	setVec3("material.specular", specular);
	setVec3("material.emissive", emissive);
	setFloat("material.shininess", shininess);
}

void Shader::setPointLight(PointLight &light, int index) {
	// TODO: When profiling this is taking up the most time, check if i can improve it
	//* CURRENTLY: setting the position for each light but keeiping the rest of the values the same
	//for each light. Not good
	//* COPILOT: This implementation is not good, i should use a uniform buffer object

	setVec3("pointLightPositions[" + std::to_string(index) + "]", light.position);
	// setVec3(name + "position", light.position);
	// setVec3(name + "ambient", light.ambient);
	// setVec3(name + "diffuse", light.diffuse);
	// setVec3(name + "specular", light.specular);
	// setFloat(name + "constant", light.constant);
	// setFloat(name + "linear", light.linear);
	// setFloat(name + "quadratic", light.quadratic);
}
void Shader::setDirectionalLight(DirectionalLight &light) {
	if (!m_isDirectionalLightSet) {
		m_isDirectionalLightSet = true;
		setVec3("dirLight.direction", light.direction);
		setVec3("dirLight.ambient", light.ambient);
		setVec3("dirLight.diffuse", light.diffuse);
		setVec3("dirLight.specular", light.specular);
	}
}
void Shader::setViewProjection(glm::mat4 &view, glm::mat4 &projection) const {
	setMat4("view", view);
	setMat4("projection", projection);
}
void Shader::setModel(glm::mat4 &model) const { setMat4("model", model); }

// Private functions
bool Shader::checkShaderCompileError(unsigned int shader, const char *type) {
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::" << type << "::COMPILATION_FAILED" << infoLog << std::endl;
	}
	return success;
}
bool Shader::checkProgramLinkError() {
	int success;
	char infoLog[512];
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(m_id, 512, nullptr, infoLog);
		std::cout << "ERROR::PROGRAM" << m_id << "::LINKING_FAILED" << infoLog << std::endl;
	}
	return success;
}