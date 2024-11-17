#include "model.h"

#include <Nexus.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "resourceManager.h"

// ======================= Triangle ========================

Triangle::Triangle(Vertex v1, Vertex v2, Vertex v3)
	: vertices({v1.position, v2.position, v3.position}) {}
float Triangle::getYmin() const { return std::min({vertices[0].y, vertices[1].y, vertices[2].y}); }
float Triangle::getYmax() const { return std::max({vertices[0].y, vertices[1].y, vertices[2].y}); }
glm::vec3 Triangle::operator[](int i) const { return vertices[i]; }

// ========================= Model =========================

Model::Model(const char *path) {
	using namespace Nexus;

	Assimp::Importer import;
	const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		Logger::error("ASSIMP::{}", import.GetErrorString());

	if (scene->mRootNode->mNumMeshes > 1)
		Logger::error(
			"Only one mesh per model is supported, {} provided. Only the first mesh will be loaded",
			scene->mRootNode->mNumMeshes);
	if (scene->mRootNode->mNumChildren > 1) Logger::error("Nested meshes is not supported");

	const auto mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[0]];

	processVertices(mesh);
	processIndices(mesh);
	processTriangles();

	initOpenGLBuffers();
}

void Model::draw(Shader &shader, glm::vec3 color) {
	shader.use();
	shader.setVec3("color", color);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}

glm::vec3 Model::getMin() const {
	glm::vec3 min = m_vertices[0].position;
	for (unsigned int i = 1; i < m_vertices.size(); ++i) {
		if (m_vertices[i].position.x < min.x) min.x = m_vertices[i].position.x;
		if (m_vertices[i].position.y < min.y) min.y = m_vertices[i].position.y;
		if (m_vertices[i].position.z < min.z) min.z = m_vertices[i].position.z;
	}
	return min;
}

glm::vec3 Model::getMax() const {
	glm::vec3 max = m_vertices[0].position;
	for (unsigned int i = 1; i < m_vertices.size(); ++i) {
		if (m_vertices[i].position.x > max.x) max.x = m_vertices[i].position.x;
		if (m_vertices[i].position.y > max.y) max.y = m_vertices[i].position.y;
		if (m_vertices[i].position.z > max.z) max.z = m_vertices[i].position.z;
	}
	return max;
}

Slice Model::getSlice(double sliceHeight) {
	std::vector<Line> lineSegments;
	for (const auto &triangle : m_triangles) {
		if (triangle.getYmin() >= sliceHeight || triangle.getYmax() <= sliceHeight) continue;

		Line segment;

		for (int i = 0; i < 3; ++i) {
			const auto &v1 = triangle[i];
			const auto &v2 = triangle[(i + 1) % 3];
			if ((v1.y - sliceHeight) * (v2.y - sliceHeight) > 0) continue;

			float t = (sliceHeight - v1.y) / (v2.y - v1.y);
			segment.setNextPoint(v1 + t * (v2 - v1));
		}
		lineSegments.push_back(segment * glm::vec3(1.0f, 0.0f, 1.0f));
	}

	return {lineSegments};
}

void Model::initOpenGLBuffers() {
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_EBO);

	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0],
				 GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint), &m_indices[0],
				 GL_STATIC_DRAW);

	// Pos
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						  (void *)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
						  (void *)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Model::processVertices(const aiMesh *mesh) {
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
		m_vertices.push_back(vertex);
	}
}

void Model::processIndices(const aiMesh *mesh) {
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			m_indices.push_back(face.mIndices[j]);
		}
	}
}

void Model::processTriangles() {
	for (size_t i = 0; i < m_indices.size(); i += 3) {
		m_triangles.emplace_back(m_vertices[m_indices[i]], m_vertices[m_indices[i + 1]],
								 m_vertices[m_indices[i + 2]]);
	}
}
