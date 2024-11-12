#pragma once
#include <glm/glm.hpp>

#include "object.h"
#include "resourceManager.h"

class Printer : public Object {
	glm::ivec3 size = {220, 250, 220};

   public:
	float nozzle = 0.4f;

	Printer(std::string model_path)
		: Object(ResourceManager::loadModel("printer", model_path.c_str()),
				 ResourceManager::getShader("shader"), glm::vec3(0.0f)) {
		scale(glm::vec3(size));
	}

	void setSize(glm::ivec3 size) {
		this->size = size;
		reset();
		scale(glm::vec3(size));
	}

	glm::ivec3 getSize() const { return size; }

	virtual glm::vec3 getCenter() const override { return glm::vec3(size) / 2.0f; }
};