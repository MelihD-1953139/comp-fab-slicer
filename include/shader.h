#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <string>

struct PointLight {
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

struct DirectionalLight {
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

class Shader {
   public:
	GLuint m_id;

	Shader();
	Shader &operator=(Shader other);
	~Shader();

	void use();
	bool addVertexShader(const char *vertexData);
	bool addFragmentShader(const char *fragmentData);
	bool linkProgram();

	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setMat4(const std::string &name, glm::mat4 value) const;
	void setVec3(const std::string &name, glm::vec3 value) const;
	void setVec4(const std::string &name, glm::vec4 value) const;
	void setMVP(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const;

   private:
	bool m_hasVertShader;
	bool m_hasFragShader;
	bool m_isLinked;
	bool m_toDeleteProgram;
	GLuint m_vertShaderId;
	GLuint m_fragShaderId;
	bool m_isDirectionalLightSet;

	bool checkShaderCompileError(unsigned int shader, const char *type);
	bool checkProgramLinkError();
};