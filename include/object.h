#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "model.h"
#include "shader.h"

class Object {
   public:
	Object(Model &model, Shader &shader, glm::vec3 pos);
	virtual void render(glm::mat4 view, glm::mat4 projection, DirectionalLight dirLight,
						glm::vec4 color);
	void scale(glm::vec3 scale);
	void scale(float scale);
	std::pair<glm::vec3, glm::vec3> getBoundingBox() const;

	virtual glm::vec3 getCenter() const;
	float getHeightOfObject() const;

	void setPosition(glm::vec3 pos);
	void setPositionCentered(glm::vec3 pos);

   protected:
	glm::vec3 m_pos;
	glm::vec3 m_scale;
	Model &m_model;
	Shader &m_shader;
};