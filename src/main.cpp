#define ZEROY glm::vec3(1.0f, 0.0f, 1.0f)

#include <Nexus.h>
#include <Nexus/Window/GLFWWindow.h>
#include <clipper2/clipper.h>

#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "camera.h"
#include "framebuffer.h"
#include "object.h"
#include "printer.h"
#include "resourceManager.h"

#define SHADER_VERT_PATH "../res/shaders/base.vert"
#define SHADER_FRAG_PATH "../res/shaders/base.frag"

using namespace Nexus;

void usage(const char* program) { Logger::info("Usage: {} <filename>", program); }

struct ControlSettings {
	glm::ivec3 printerSize = {220, 250, 220};  // Initial printer size (example)
	float printNozzle = 0.4f;
	float objectHeight = 0.0f;
	int sliceIndex = 0;
	float maxSliceIndex = 0.0f;
	bool updateSlice = false;
};

struct State {
	bool slice;
	bool showDemoWindow;
	std::pair<int, int> windowSize;
	float layerHeight;
	int maxSliceIndex;
	int sliceIndex;
};

int main(int argc, char* argv[]) {
	Logger::setLevel(LogLevel::Trace);

	if (argc < 2) {
		Logger::critical("Invalid number of arguments");
		usage(argv[0]);
		return 1;
	}

	auto window = std::unique_ptr<Window>(Window::create());
	window->setVSync(true);

	auto shader = ResourceManager::loadShader("shader", SHADER_VERT_PATH, SHADER_FRAG_PATH);

	Object plane(ResourceManager::loadModel("plane", "../res/models/plane.obj"), shader);

	Printer printer("../res/models/plane.obj");
	Object model(ResourceManager::loadModel("model", argv[1]), shader);
	model.setPositionCentered(printer.getCenter() * ZEROY);

	PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
	OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

	State state{
		.slice = false,
		.showDemoWindow = false,
		.windowSize{1280, 720},
		.layerHeight = 0.2f,
		.maxSliceIndex = static_cast<int>(std::ceil(model.getHeightOfObject() / state.layerHeight)),
		.sliceIndex = state.maxSliceIndex,
	};

	Framebuffer viewBuffer(state.windowSize.first, state.windowSize.second);
	Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

	window
		->onKey([&](int key, int scancode, int action, int mods) -> bool {
			if (key == GLFW_KEY_W && action != GLFW_RELEASE) camera.orbit(0.0f, -1.0f);
			if (key == GLFW_KEY_S && action != GLFW_RELEASE) camera.orbit(0.0f, 1.0f);
			if (key == GLFW_KEY_A && action != GLFW_RELEASE) camera.orbit(-1.0f, 0.0f);
			if (key == GLFW_KEY_D && action != GLFW_RELEASE) camera.orbit(1.0f, 0.0f);
			if (key == GLFW_KEY_SPACE && action != GLFW_RELEASE)
				state.showDemoWindow = !state.showDemoWindow;

			if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
				camera.offsetDistanceFromTarget(-5.0f);
			if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
				camera.offsetDistanceFromTarget(5.0f);
			return false;
		})
		->onResize([&](int width, int height) -> bool {
			state.windowSize = {width, height};
			state.slice = true;
			return false;
		});

	window->whileOpen([&]() {
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		if (state.showDemoWindow) ImGui::ShowDemoWindow();

		ImGui::Begin("Control Panel");
		{
			if (ImGui::CollapsingHeader("Printer settings")) {
				int printerSize[3] = {printer.getSize().x, printer.getSize().y,
									  printer.getSize().z};
				if (ImGui::InputInt3("Printer Size (mm)", printerSize)) {
					printer.setSize({printerSize[0], printerSize[1], printerSize[2]});
					model.setPositionCentered(printer.getCenter() * ZEROY);
				}

				ImGui::InputFloat("##Printer Nozel", &printer.nozzle, 0.0f, 0.0f, "%.2f mm");
			}

			// if (ImGui::CollapsingHeader("Object settings")) {
			// }

			if (ImGui::CollapsingHeader("Slice settings")) {
				ImGui::SliderInt("Slice Index", &state.sliceIndex, 1, state.maxSliceIndex);

				if (ImGui::InputFloat("Layer Height", &state.layerHeight, 0.0f, 0.0f, "%.2f mm"))
					state.layerHeight = std::clamp(state.layerHeight, 0.0f, printer.nozzle * 0.8f);
			}

			state.slice = ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0));
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		{
			ImGui::Begin("3D View");
			{
				const int width = ImGui::GetContentRegionAvail().x;
				const int height = ImGui::GetContentRegionAvail().y;
				auto view = camera.getViewMatrix(printer.getCenter() * ZEROY);
				auto projection = camera.getProjectionMatrix(width, height);

				plane.setPositionCentered(
					printer.getCenter() * ZEROY +
					glm::vec3(0.0f, state.sliceIndex * state.layerHeight, 0.0f));
				plane.scale(printer.getSize());

				viewBuffer.bind();
				{
					viewBuffer.resize(width, height);
					glViewport(0, 0, width, height);
					viewBuffer.clear();

					printer.render(view, projection, glm::vec3(0.7f, 0.7f, 0.7f));
					plane.render(view, projection, glm::vec3(0.0f, 0.0f, 1.0f));
					model.render(view, projection, glm::vec3(1.0f, 0.0f, 0.0f));
				}
				viewBuffer.unbind();

				ImGui::Image((void*)(intptr_t)viewBuffer.getTexture(),
							 ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
			}
			ImGui::End();

			ImGui::Begin("Slice View");
			{
				sliceBuffer.bind();
				if (state.slice) {
					const int width = ImGui::GetContentRegionAvail().x;
					const int height = ImGui::GetContentRegionAvail().y;
					auto view = topDownCamera.getViewMatrix(printer.getCenter() * ZEROY);
					auto projection = topDownCamera.getProjectionMatrix(width, height);
					sliceBuffer.resize(width, height);
					glViewport(0, 0, width, height);
					sliceBuffer.clear();

					auto slice = model.getSlice(state.sliceIndex * state.layerHeight + 0.000000001);
					if (slice)
						slice.value().render(shader, view, projection, glm::vec3(1.0f, 0.0f, 0.0f));
				}
				sliceBuffer.unbind();

				ImGui::Image((void*)(intptr_t)sliceBuffer.getTexture(),
							 ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
			}
			ImGui::End();
		}
		ImGui::PopStyleVar();
	});
}