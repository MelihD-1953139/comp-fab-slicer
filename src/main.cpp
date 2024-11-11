#include <cmath>
#include <glm/gtc/type_ptr.hpp>	 // Include this for glm::value_ptr()
#include <iostream>
#include <memory>

#include "Nexus.h"
#include "Nexus/Window/GLFWWindow.h"
#include "camera.h"
#include "object.h"
#include "resourceManager.h"

using namespace Nexus;

void usage(const char* program) { std::cerr << "Usage: " << program << " <filename>" << std::endl; }

struct ControlSettings {
	glm::ivec3 printerSize = {220, 250, 220};	   // Initial printer size (example)
	glm::ivec3 scaleFactorObject = glm::ivec3(1);  // Initial scale factor (example)
	float printNozzle = 0.4f;
	float objectHeight = 0.0f;
	int sliceIndex = 0;
	float maxSliceIndex = 0.0f;
};

void showControlPanel(ControlSettings& settings) {
	ImGui::Begin("Control Panel");
	// ImGui::SliderFloat("Height Slice Plane", &slicePlane, 0.0f, 1.0f);
	ImGui::Text("Printer settings");

	int printerSize[3] = {settings.printerSize[0], settings.printerSize[1],
						  settings.printerSize[2]};
	ImGui::InputInt3("Printer Size (mm)", printerSize);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		settings.printerSize = glm::ivec3(printerSize[0], printerSize[1], printerSize[2]);
		Logger::debug("Printer size changed: [{}, {}, {}]", settings.printerSize[0],
					  settings.printerSize[1], settings.printerSize[2]);
		// plane.scale(settings.printerSize);
	}

	ImGui::SliderInt("Slice Index", &settings.sliceIndex, 0, settings.maxSliceIndex);

	int scaleFactorObject[3] = {settings.scaleFactorObject[0], settings.scaleFactorObject[1],
								settings.scaleFactorObject[2]};
	ImGui::InputInt3("Scale factor (x, y, z)", scaleFactorObject);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		settings.scaleFactorObject =
			glm::ivec3(scaleFactorObject[0], scaleFactorObject[1], scaleFactorObject[2]);
		Logger::debug("Scale Factor Changed: [{}, {}, {}]", settings.scaleFactorObject[0],
					  settings.scaleFactorObject[1], settings.scaleFactorObject[2]);
		// plane.scale(glm::vec3(scaleFactorObject[0], scaleFactorObject[1],
		// scaleFactorObject[2]));
	}
	// ImGui::Text("Plane center: (%.2f, %.2f, %.2f)", plane.getCenter().x, plane.getCenter().y,
	// 			plane.getCenter().z);
	// ImGui::Text("Height object: %.2f", cube.getHeightOfObject());
	ImGui::End();
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	auto window = std::unique_ptr<Window>(Window::create());

	window->setVSync(true);

	auto shader =
		ResourceManager::loadShader("shader", "res/shaders/base.vert", "res/shaders/base.frag");
	ResourceManager::loadModel("model", argv[1]);
	ResourceManager::loadModel("plane", "res/models/plane.obj");
	ResourceManager::loadModel("sphere", "res/models/sphere.obj");

	Object model(ResourceManager::getModel("model"), shader, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
	Object plane(ResourceManager::getModel("plane"), shader, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);

	Object sphere(ResourceManager::getModel("sphere"), shader, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);

	Camera camera(500.0f);
	window->onKey([&camera](int key, int scancode, int action, int mods) -> bool {
		if (key == GLFW_KEY_W && action != GLFW_RELEASE) camera.move(FORWARD, 1.0f);
		if (key == GLFW_KEY_S && action != GLFW_RELEASE) camera.move(BACKWARD, 1.0f);
		if (key == GLFW_KEY_A && action != GLFW_RELEASE) camera.move(LEFT, 1.0f);
		if (key == GLFW_KEY_D && action != GLFW_RELEASE) camera.move(RIGHT, 1.0f);
		if (key == GLFW_KEY_UP && action != GLFW_RELEASE) camera.offsetDistanceFromTarget(-0.5f);
		if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) camera.offsetDistanceFromTarget(0.5f);
		return false;
	});

	glClearColor(0.5, 0.5, 0.5, 1.0);

	ControlSettings settings;

	settings.objectHeight = model.getHeightOfObject();
	settings.maxSliceIndex = std::ceil(settings.objectHeight / settings.printNozzle);
	plane.scale(settings.printerSize);

	// Run the main loop
	while (!window->shouldClose) {
		window->frameStart();
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::ShowDemoWindow();
		showControlPanel(settings);

		auto view = camera.getViewMatrix(plane.getCenter());
		auto projection = camera.getProjectionMatrix(1280, 720);
		DirectionalLight dirLight;

		plane.render(view, projection, dirLight, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		model.render(view, projection, dirLight, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		window->frameEnd();
	}
}