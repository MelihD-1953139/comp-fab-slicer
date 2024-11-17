#include "resourceManager.h"

#include <fstream>
#include <iostream>
#include <sstream>

std::map<std::string, Shader> ResourceManager::Shaders;
std::map<std::string, Model> ResourceManager::Models;

Shader &ResourceManager::loadShader(std::string name, std::string vertexPath,
									std::string fragmentPath) {
	Shaders[name] = loadShadersFromFile(vertexPath.c_str(), fragmentPath.c_str());
	printf("Loaded shader %s, with id %d\n", name.c_str(), Shaders[name].m_id);
	return Shaders[name];
}
Shader &ResourceManager::getShader(std::string name) {
	printf("Got shader %s, with id %d\n", name.c_str(), Shaders[name].m_id);
	return Shaders[name];
}

Model &ResourceManager::loadModel(std::string name, std::string filepath, bool isInstanced) {
	Models.emplace(name, filepath.c_str());
	return Models.find(name)->second;
}
Model &ResourceManager::getModel(std::string name) { return Models.find(name)->second; }

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