#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glad/gl.h>

struct PointLight
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

class Shader
{
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
	void setMaterial(glm::vec3 &ambient, glm::vec3 &diffuse, glm::vec3 &specular, glm::vec3 &emissive, float shininess) const;
	void setPointLight(PointLight &light, int index);
	void setDirectionalLight(DirectionalLight &light);
	void setViewProjection(glm::mat4 &view, glm::mat4 &projection) const;
	void setModel(glm::mat4 &model) const;

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