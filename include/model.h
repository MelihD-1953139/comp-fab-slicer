#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <vector>

#include "mesh.h"
#include "shader.h"

class Model {
   public:
	Model();
	Model(const char *path);
	~Model();

	void draw(Shader &shader, glm::vec4 color);

	std::vector<Mesh> getMeshes() { return m_meshes; }
	std::string getName() { return m_name; }
	std::string getDirectory() { return m_directory; }
	glm::vec3 getMin() const;
	glm::vec3 getMax() const;

   private:
	std::vector<Mesh> m_meshes;
	std::string m_name;
	std::string m_directory;

	void loadModel(std::string path);
	void processNode(aiNode *node, const aiScene *scene);
	Mesh processMesh(aiMesh *mesh, const aiScene *scene);
	std::vector<Vertex> extractVertices(aiMesh *mesh);
	std::vector<unsigned int> extractIndices(aiMesh *mesh);
};