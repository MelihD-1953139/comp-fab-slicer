#include "model.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "resourceManager.h"

// Public
Model::Model() {
	m_meshes = std::vector<Mesh>();
	m_name = "";
	m_directory = "";
}

Model::Model(const char *path) { loadModel(path); }

Model::~Model() { m_meshes.clear(); }

void Model::draw(Shader &shader, glm::vec3 color) {
	for (Mesh &mesh : m_meshes) {
		mesh.draw(shader, color);
	}
}

glm::vec3 Model::getMin() const {
	glm::vec3 min = glm::vec3(INFINITY);
	for (Mesh mesh : m_meshes) {
		glm::vec3 meshMin = mesh.getMin();
		if (meshMin.x < min.x) min.x = meshMin.x;
		if (meshMin.y < min.y) min.y = meshMin.y;
		if (meshMin.z < min.z) min.z = meshMin.z;
	}
	return min;
}

glm::vec3 Model::getMax() const {
	glm::vec3 max = glm::vec3(-INFINITY);
	for (Mesh mesh : m_meshes) {
		glm::vec3 meshMax = mesh.getMax();
		if (meshMax.x > max.x) max.x = meshMax.x;
		if (meshMax.y > max.y) max.y = meshMax.y;
		if (meshMax.z > max.z) max.z = meshMax.z;
	}
	return max;
}

// ========================= Private =========================
void Model::loadModel(std::string path) {
	Assimp::Importer import;
	const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
	}

	size_t start = path.find_last_of('/') + 1;
	size_t dot = path.find_last_of('.');
	m_name = path.substr(start, dot - start);
	m_directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}
void Model::processNode(aiNode *node, const aiScene *scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
	std::vector<Vertex> vertices = extractVertices(mesh);
	std::vector<unsigned int> indices = extractIndices(mesh);

	return Mesh(vertices, indices);
}

std::vector<Vertex> Model::extractVertices(aiMesh *mesh) {
	std::vector<Vertex> vertices;
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;

		if (mesh->HasNormals()) {
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;
		}
		vertices.push_back(vertex);
	}
	return vertices;
}

std::vector<unsigned int> Model::extractIndices(aiMesh *mesh) {
	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}
	return indices;
}

std::optional<Slice> Model::getSlice(double sliceHeight) {
	assert(m_meshes.size() == 1 && "Only one mesh is supported for slicing");
	return m_meshes[0].getSlice(sliceHeight);
}