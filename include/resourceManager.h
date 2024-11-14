#pragma once

#include <map>
#include <string>

#include "model.h"
#include "shader.h"

class ResourceManager {
   public:
	static Shader &loadShader(std::string name, std::string vertexPath, std::string fragmentPath);
	static Shader &getShader(std::string name);

	static Model &loadModel(std::string name, std::string filepath, bool isInstanced = false);
	static Model &getModel(std::string name);

	static void clear();

	static std::map<std::string, Shader> Shaders;
	static std::map<std::string, Model> Models;

   private:
	ResourceManager() {};

	static Shader loadShadersFromFile(const char *vertexPath, const char *fragmentPath);

	static std::string readFile(const char *filePath);

	static GLenum determineFormat(unsigned int nrChannels);
};