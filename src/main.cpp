#include <iostream>

#include "Nexus.h"
#include "Nexus/Window/GLFWWindow.h"

void test() {
	auto window = Nexus::Window::create();
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	window->setVSync(true);
	window->onKey([window](int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			Nexus::Logger::info("Escape key pressed");
			window->shouldClose = true;
			return true;
		}
		return false;
	});

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!window->shouldClose) {
		window->frameStart();
		glClear(GL_COLOR_BUFFER_BIT);

		//* Stolen from ImGui example
		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You
		// can browse its code to learn more about Dear ImGui!).
		if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a
		// named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin(
				"Hello, world!");  // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");  // Display some text (you can use a format
													   // strings too)
			ImGui::Checkbox("Demo Window",
							&show_demo_window);	 // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f,
							   1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color",
							  (float*)&clear_color);  // Edit 3 floats representing a color

			if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return
										  // true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
						1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window) {
			ImGui::Begin(
				"Another Window",
				&show_another_window);	// Pass a pointer to our bool variable (the window will have
										// a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me")) show_another_window = false;
			ImGui::End();
		}

		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
					 clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		window->frameEnd();
	}

	delete window;
}

/*#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/gl.h>

#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <vector>

// Vertex structure
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

// Mesh class
class Mesh {
   public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
		this->vertices = vertices;
		this->indices = indices;
		setupMesh();
	}

	void Draw() {
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

   private:
	unsigned int VAO, VBO, EBO;

	void setupMesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		// Load data into vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0],
					 GL_STATIC_DRAW);

		// Load indices into element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0],
					 GL_STATIC_DRAW);

		// Vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		// Vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void*)offsetof(Vertex, normal));

		// Vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void*)offsetof(Vertex, texCoords));

		glBindVertexArray(0);
	}
};

std::vector<Mesh> meshes;

Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	// Process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;
		glm::vec3 vector;

		// Positions
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;

		// Normals
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.normal = vector;

		vertices.push_back(vertex);
	}

	// Process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	return Mesh(vertices, indices);
}

void processNode(aiNode* node, const aiScene* scene) {
	// Process all the node's meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}

	// Recursively process children
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processNode(node->mChildren[i], scene);
	}
}
void loadModel(const std::string& path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	processNode(scene->mRootNode, scene);
}

void render() {
	// Render all meshes
	for (unsigned int i = 0; i < meshes.size(); i++) {
		meshes[i].Draw();
	}
}*/

#include "camera.h"
#include "object.h"
#include "resourceManager.h"

int main() {
	auto window = Nexus::Window::create();

	window->setVSync(true);
	window->onKey([window](int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			Nexus::Logger::info("Escape key pressed");
			window->shouldClose = true;
			return true;
		}
		return false;
	});

	auto shader =
		ResourceManager::loadShader("shader", "res/shaders/base.vert", "res/shaders/base.frag");
	ResourceManager::loadModel("model", "res/models/cube.stl");

	Object object(ResourceManager::getModel("model"), shader, glm::vec3(0.0f, 0.0f, 0.0f));

	Camera camera{};
	window
		->onMouseMove([&camera](double x, double y) -> bool {
			static double lastX = x, lastY = y;

			float xoffset = x - lastX;
			float yoffset = lastY - y;

			lastX = x;
			lastY = y;

			camera.processLookingDirection(xoffset, yoffset);
			return false;
		})
		->onKey([&camera](int key, int scancode, int action, int mods) -> bool {
			if (key == GLFW_KEY_W && action != GLFW_RELEASE) camera.move(FORWARD, 0.1f);
			if (key == GLFW_KEY_S && action != GLFW_RELEASE) camera.move(BACKWARD, 0.1f);
			if (key == GLFW_KEY_A && action != GLFW_RELEASE) camera.move(LEFT, 0.1f);
			if (key == GLFW_KEY_D && action != GLFW_RELEASE) camera.move(RIGHT, 0.1f);
			return false;
		});

	glClearColor(0.3, 0.3, 0.3, 1.0);

	// Run the main loop
	while (!window->shouldClose) {
		window->frameStart();
		glClear(GL_COLOR_BUFFER_BIT);

		object.render(camera.getViewMatrix(), camera.getProjectionMatrix(1280, 720),
					  DirectionalLight(), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		window->frameEnd();
	}

	delete window;
}