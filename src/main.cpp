#define GLM_ENABLE_EXPERIMENTAL
#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "Nexus.h"
#include "Nexus/Window/GLFWWindow.h"
#include "camera.h"
#include "framebuffer.h"
#include "object.h"
#include "printer.h"
#include "resourceManager.h"

using namespace Nexus;

void usage(const char* program) {
	Logger::info("Usage: {} <path_to_base_folder> <filename>", program);
}

struct ControlSettings {
	glm::ivec3 printerSize = {220, 250, 220};		// Initial printer size (example)
	glm::vec3 scaleFactorObject = glm::vec3(1.0f);	// Initial scale factor (example)
	float printNozzle = 0.4f;
	float objectHeight = 0.0f;
	int sliceIndex = 0;
	float maxSliceIndex = 0.0f;
	bool updateSlice = false;
};

void showControlPanel(ControlSettings& settings, Printer& printer, Object& model) {
	ImGui::Begin("Control Panel");
	ImGui::Text("Printer settings");

	int printerSize[3] = {printer.getSize().x, printer.getSize().y, printer.getSize().z};
	if (ImGui::InputInt3("Printer Size (mm)", printerSize)) {
		printer.setSize({printerSize[0], printerSize[1], printerSize[2]});
		model.setPositionCentered(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));
	}

	if (ImGui::SliderInt("Slice Index", &settings.sliceIndex, 1, settings.maxSliceIndex))
		settings.updateSlice = true;

	float scaleFactorObject[3] = {settings.scaleFactorObject[0], settings.scaleFactorObject[1],
								  settings.scaleFactorObject[2]};
	if (ImGui::InputFloat3("Scale factor (x, y, z)", scaleFactorObject)) {
		settings.scaleFactorObject =
			glm::vec3(scaleFactorObject[0], scaleFactorObject[1], scaleFactorObject[2]);
		model.scale(settings.scaleFactorObject);
	}

	if (ImGui::Button("Slice")) {
		model.getSlice(settings.sliceIndex * settings.printNozzle + 0.000000001);
	}
	ImGui::End();
}

int main(int argc, char* argv[]) {
	Logger::setLevel(LogLevel::Trace);

	if (argc != 3) {
		Logger::critical("Invalid number of arguments");
		usage(argv[0]);
		return 1;
	}

	auto window = std::unique_ptr<Window>(Window::create());

	window->setVSync(true);

	auto path = std::string(argv[1]);

	auto shader = ResourceManager::loadShader("shader", path + "/res/shaders/base.vert",
											  path + "/res/shaders/base.frag");
	ResourceManager::loadModel("model", path + "/" + argv[2]);
	ResourceManager::loadModel("sphere", path + "/res/models/sphere.obj");

	Object plane(ResourceManager::loadModel("plane", path + "/res/models/plane.obj"), shader,
				 glm::vec3(0.0f));

	Object model(ResourceManager::getModel("model"), shader, glm::vec3(0.0f, 0.0f, 0.0f));
	Printer printer(path + "/res/models/plane.obj");
	model.setPositionCentered(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));

	PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
	OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

	std::pair<int, int> windowSize{1280, 720};

	bool show_demo_window = false;

	window
		->onKey([&camera, &show_demo_window](int key, int scancode, int action, int mods) -> bool {
			glm::vec2 offset = {0.0f, 0.0f};  // Offset for orbiting
			if (key == GLFW_KEY_W && action != GLFW_RELEASE) camera.orbit(0.0f, -1.0f);
			if (key == GLFW_KEY_S && action != GLFW_RELEASE) camera.orbit(0.0f, 1.0f);
			if (key == GLFW_KEY_A && action != GLFW_RELEASE) camera.orbit(-1.0f, 0.0f);
			if (key == GLFW_KEY_D && action != GLFW_RELEASE) camera.orbit(1.0f, 0.0f);
			if (key == GLFW_KEY_SPACE && action != GLFW_RELEASE)
				show_demo_window = !show_demo_window;

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

	ControlSettings settings;

	settings.objectHeight = model.getHeightOfObject();
	settings.maxSliceIndex = std::ceil(settings.objectHeight / settings.printNozzle);
	settings.sliceIndex = settings.maxSliceIndex;

	Framebuffer viewBuffer(windowSize.first, windowSize.second);
	Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

	// Run the main loop
	window->whileOpen([&]() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		if (show_demo_window) ImGui::ShowDemoWindow();
		showControlPanel(settings, printer, model);

		plane.setPositionCentered(
			printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f) +
			glm::vec3(0.0f, settings.sliceIndex * settings.printNozzle, 0.0f));
		plane.scale(printer.getSize());

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("3D View");
		{
			const int width = ImGui::GetContentRegionAvail().x;
			const int height = ImGui::GetContentRegionAvail().y;
			auto view = camera.getViewMatrix(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));
			auto projection = camera.getProjectionMatrix(width, height);

			viewBuffer.bind();
			{
				viewBuffer.resize(width, height);
				glViewport(0, 0, width, height);
				viewBuffer.clear();

				printer.render(view, projection, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
				plane.render(view, projection, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
				model.render(view, projection, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			viewBuffer.unbind();

			ImGui::Image((void*)(intptr_t)viewBuffer.getTexture(), ImGui::GetContentRegionAvail(),
						 ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::End();

		ImGui::Begin("Slice View");
		{
			const int width = ImGui::GetContentRegionAvail().x;
			const int height = ImGui::GetContentRegionAvail().y;

			auto view =
				topDownCamera.getViewMatrix(printer.getCenter() * glm::vec3(1.0f, 0.0f, 1.0f));
			auto projection = topDownCamera.getProjectionMatrix(width, height);

			sliceBuffer.bind();
			{
				sliceBuffer.resize(width, height);
				glViewport(0, 0, width, height);
				sliceBuffer.clear();
				shader.setViewProjection(view, projection);

				// printer.render(view, projection, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));

				// model.getSlice(settings.sliceIndex * settings.printNozzle + 0.000000001)
				// 	.draw(shader, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			}
			sliceBuffer.unbind();

			ImGui::Image((void*)(intptr_t)sliceBuffer.getTexture(), ImGui::GetContentRegionAvail(),
						 ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::End();
		ImGui::PopStyleVar();
	});
}