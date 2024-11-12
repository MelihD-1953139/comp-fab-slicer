#define GLM_ENABLE_EXPERIMENTAL
#include <clipper2/clipper.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>

#include "Nexus.h"
#include "Nexus/Window/GLFWWindow.h"
#include "camera.h"
#include "object.h"
#include "printer.h"
#include "resourceManager.h"

using namespace Nexus;

void usage(const char* program) { Logger::info("Usage: {} <filename>", program); }

struct ControlSettings {
	glm::ivec3 printerSize = {220, 250, 220};	   // Initial printer size (example)
	glm::ivec3 scaleFactorObject = glm::ivec3(1);  // Initial scale factor (example)
	float printNozzle = 0.4f;
	float objectHeight = 0.0f;
	int sliceIndex = 0;
	float maxSliceIndex = 0.0f;
};

void showControlPanel(ControlSettings& settings, Printer& printer, Object& model) {
	ImGui::Begin("Control Panel");
	// ImGui::SliderFloat("Height Slice Plane", &slicePlane, 0.0f, 1.0f);
	ImGui::Text("Printer settings");

	int printerSize[3] = {printer.getSize().x, printer.getSize().y, printer.getSize().z};
	if (ImGui::InputInt3("Printer Size (mm)", printerSize)) {
		Logger::debug("Printer size changed: [{}, {}, {}]", printerSize[0], printerSize[1],
					  printerSize[2]);

		printer.setSize({printerSize[0], printerSize[1], printerSize[2]});
		model.setPositionCentered(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));
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
	ImGui::End();
}

int main(int argc, char* argv[]) {
	Logger::setLevel(LogLevel::Trace);

	if (argc != 2) {
		Logger::critical("Invalid number of arguments");
		usage(argv[0]);
		return 1;
	}

	auto window = std::unique_ptr<Window>(Window::create());

	window->setVSync(true);

	auto shader =
		ResourceManager::loadShader("shader", "res/shaders/base.vert", "res/shaders/base.frag");
	ResourceManager::loadModel("model", argv[1]);
	ResourceManager::loadModel("sphere", "res/models/sphere.obj");

	Object plane(ResourceManager::loadModel("plane", "res/models/plane.obj"), shader,
				 glm::vec3(0.0f, 0.0f, 0.0f));

	Object model(ResourceManager::getModel("model"), shader, glm::vec3(0.0f, 0.0f, 0.0f));
	Printer printer("res/models/plane.obj");
	model.setPositionCentered(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));

	Camera camera(300.0f);
	Logger::debug("Camera position: {}", glm::to_string(camera.getPosition()));
	std::pair<int, int> windowSize{1280, 720};

	window
		->onKey([&camera](int key, int scancode, int action, int mods) -> bool {
			glm::vec2 offset = {0.0f, 0.0f};  // Offset for orbiting
			if (key == GLFW_KEY_W && action != GLFW_RELEASE) camera.orbit(0.0f, -1.0f);
			if (key == GLFW_KEY_S && action != GLFW_RELEASE) camera.orbit(0.0f, 1.0f);
			if (key == GLFW_KEY_A && action != GLFW_RELEASE) camera.orbit(-1.0f, 0.0f);
			if (key == GLFW_KEY_D && action != GLFW_RELEASE) camera.orbit(1.0f, 0.0f);

			if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
				camera.offsetDistanceFromTarget(-5.0f);
			if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
				camera.offsetDistanceFromTarget(5.0f);
			return false;
		})
		->onResize([&camera, &windowSize](int width, int height) -> bool {
			windowSize = {width, height};
			return false;
		});

	glClearColor(0.98, 0.98, 0.98, 1.0);
	glEnable(GL_DEPTH_TEST);

	ControlSettings settings;

	settings.objectHeight = model.getHeightOfObject();
	settings.maxSliceIndex = std::ceil(settings.objectHeight / settings.printNozzle);

	DirectionalLight dirLight{
		.direction = glm::vec3(1.0f, -1.0f, 0.0f),
		.ambient = glm::vec3(0.2f, 0.2f, 0.2f),
	};

	// Run the main loop
	window->whileOpen([&]() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui::ShowDemoWindow();
		showControlPanel(settings, printer, model);

		auto view = camera.getViewMatrix(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));
		auto projection = camera.getProjectionMatrix(windowSize.first, windowSize.second);

		plane.setPositionCentered(
			printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f) +
			glm::vec3(0.0f, settings.sliceIndex * settings.printNozzle, 0.0f));
		plane.scale(printer.getSize());

		printer.render(view, projection, dirLight, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
		plane.render(view, projection, dirLight, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		model.render(view, projection, dirLight, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	});
}