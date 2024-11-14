#include "resourceManager.h"

#include <fstream>
#include <iostream>
#include <sstream>

#define DEBUG_GET 0
#if DEBUG_GET
#define DEBUG_GET_PRINT(x, y) printf(x, y)
#else
#define DEBUG_GET_PRINT(x, y)
#endif
#define DEBUG_LOAD 0
#if DEBUG_LOAD
#define DEBUG_LOAD_PRINT(x, y) printf(x, y)
#else
#define DEBUG_LOAD_PRINT(x, y)
#endif

std::map<std::string, Shader> ResourceManager::Shaders;
std::map<std::string, Model> ResourceManager::Models;

Shader &ResourceManager::loadShader(std::string name, std::string vertexPath,
									std::string fragmentPath) {
	DEBUG_LOAD_PRINT("Loading shader %s\n", name.c_str());
	Shaders[name] = loadShadersFromFile(vertexPath.c_str(), fragmentPath.c_str());
	printf("Loaded shader %s, with id %d\n", name.c_str(), Shaders[name].m_id);
	return Shaders[name];
}
Shader &ResourceManager::getShader(std::string name) {
	DEBUG_GET_PRINT("Getting shader %s\n", name.c_str());
	printf("Got shader %s, with id %d\n", name.c_str(), Shaders[name].m_id);
	return Shaders[name];
}

Model &ResourceManager::loadModel(std::string name, std::string filepath, bool isInstanced) {
	DEBUG_LOAD_PRINT("Loading model %s\n", name.c_str());
	Models[name] = std::move(Model(filepath.c_str()));
	return Models[name];
}
Model &ResourceManager::getModel(std::string name) {
	DEBUG_GET_PRINT("Getting model %s\n", name.c_str());
	return Models[name];
}

void ResourceManager::clear() {
	Shaders.clear();
	Models.clear();
}

Shader ResourceManager::loadShadersFromFile(const char *vertexPath, const char *fragmentPath) {
	Shader toReturn;
	toReturn.addVertexShader(readFile(vertexPath).c_str());
	toReturn.addFragmentShader(readFile(fragmentPath).c_str());
	toReturn.linkProgram();

	return toReturn;
}

std::string ResourceManager::readFile(const char *filePath) {
	std::string content;
	std::ifstream filestream(filePath, std::ios::in);

	if (!filestream.is_open()) {
		std::cout << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}

	std::string line = "";
	while (!filestream.eof()) {
		std::getline(filestream, line);
		content.append(line + "\n");
	}

	filestream.close();
	return content;
}

GLenum ResourceManager::determineFormat(unsigned int nrChannels) {
	switch (nrChannels) {
		case 1:
			return GL_RED;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			printf("Number of channels not defined");
			assert(nrChannels <= 4);
			return 0;
	}
}