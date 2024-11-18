#include "Nexus/Log.h"
#include "clipper2/clipper.core.h"
#include "clipper2/clipper.offset.h"
#define ZEROY glm::vec3(1.0f, 0.0f, 1.0f)

#include "camera.h"
#include "framebuffer.h"
#include "gcodeWriter.h"
#include "printer.h"

#include <Nexus.h>
#include <Nexus/Window/GLFWWindow.h>
#include <clipper2/clipper.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>

#define SHADER_VERT_PATH "../res/shaders/base.vert"
#define SHADER_FRAG_PATH "../res/shaders/base.frag"
#define SLICEVIEW_VERT_PATH "../res/shaders/sliceview.vert"

using namespace Nexus;
using namespace Clipper2Lib;

void usage(const char *program) {
  Logger::info("Usage: {} <filename>", program);
}

struct State {
  bool slice;
  bool showDemoWindow;
  std::pair<int, int> windowSize;
  float layerHeight;
  int maxSliceIndex;
  int sliceIndex;
  char fileBuffer[256];
  std::vector<Slice> slices;
  std::vector<Clipper2Lib::PathsD> paths;
};

int main(int argc, char *argv[]) {
  Logger::setLevel(LogLevel::Trace);

  if (argc < 2) {
    Logger::critical("Invalid number of arguments");
    usage(argv[0]);
    return 1;
  }

  auto window = std::unique_ptr<Window>(Window::create());
  window->setVSync(true);

  Shader shader(SHADER_VERT_PATH, SHADER_FRAG_PATH);

  Printer printer("../res/models/plane.obj", "../res/models/plane.obj");
  Model model(argv[1]);
  model.setPositionCentered(printer.getCenter() * ZEROY);

  PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
  OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

  State state{
      .slice = false,
      .showDemoWindow = false,
      .windowSize{1280, 720},
      .layerHeight = 0.2f,
      .maxSliceIndex =
          static_cast<int>(std::ceil(model.getHeight() / state.layerHeight)),
      .sliceIndex = 0,
      .fileBuffer = "",
  };

  strcpy(state.fileBuffer, argv[1]);
  printer.setSliceHeight(state.layerHeight * state.sliceIndex);

  Framebuffer viewBuffer(state.windowSize.first, state.windowSize.second);
  Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

  // Callbacks
  window
      ->onKey([&](int key, int scancode, int action, int mods) -> bool {
        if (key == GLFW_KEY_W && action != GLFW_RELEASE)
          camera.orbit(0.0f, -1.0f);
        if (key == GLFW_KEY_S && action != GLFW_RELEASE)
          camera.orbit(0.0f, 1.0f);
        if (key == GLFW_KEY_A && action != GLFW_RELEASE)
          camera.orbit(-1.0f, 0.0f);
        if (key == GLFW_KEY_D && action != GLFW_RELEASE)
          camera.orbit(1.0f, 0.0f);
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

  // Main loop
  window->whileOpen([&]() {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (state.showDemoWindow)
      ImGui::ShowDemoWindow();

    ImGui::Begin("Control Panel");
    {
      if (ImGui::CollapsingHeader("Printer settings")) {
        int printerSize[3] = {printer.getSize().x, printer.getSize().y,
                              printer.getSize().z};
        if (ImGui::InputInt3("Printer Size (mm)", printerSize)) {
          printer.setSize({printerSize[0], printerSize[1], printerSize[2]});
          model.setPositionCentered(printer.getCenter() * ZEROY);
        }

        ImGui::InputFloat("Printer Nozel", printer.getNozzlePtr(), 0.0f, 0.0f,
                          "%.2f mm");
      }

      if (ImGui::CollapsingHeader("Object settings")) {
        ImGui::InputText("Model file", state.fileBuffer,
                         IM_ARRAYSIZE(state.fileBuffer));
        if (ImGui::Button("Load")) {
          model = Model(state.fileBuffer);
          model.setPositionCentered(printer.getCenter() * ZEROY);
        }
      }

      if (ImGui::CollapsingHeader("Slice settings")) {
        if (ImGui::SliderInt("Slice Index", &state.sliceIndex, 1,
                             state.maxSliceIndex)) {
          printer.setSliceHeight(state.sliceIndex * state.layerHeight);
        }

        if (ImGui::InputFloat("Layer Height", &state.layerHeight, 0.0f, 0.0f,
                              "%.2f mm")) {
          state.layerHeight =
              std::clamp(state.layerHeight, 0.0f, printer.getNozzle() * 0.8f);
          state.maxSliceIndex = static_cast<int>(
              std::ceil(model.getHeight() / state.layerHeight));
        }
      }

      if (ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {

        state.slices.clear();
        for (int i = 0; i < state.maxSliceIndex; ++i) {
          auto slice = model.getSlice(state.layerHeight * i + 0.000000001);
          auto paths = Union(slice, Clipper2Lib::FillRule::EvenOdd);
          paths = InflatePaths(paths, -printer.getNozzle() / 2.0f,
                               JoinType::Miter, EndType::Polygon);

          state.paths.push_back(paths);
        }
        // TODO do slicing stuff
        for (auto &paths : state.paths)
          state.slices.push_back(paths);
      }
      if (ImGui::Button("Export to g-code",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        GcodeWriter::NewGcodeFile("output.gcode");
        GcodeWriter::WriteHeader();
        GcodeWriter::WriteSlice(state.slices.back(), state.layerHeight,
                                printer.getNozzle());
        GcodeWriter::WriteFooter();
        GcodeWriter::CloseGcodeFile();
      }
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

        viewBuffer.bind();
        {
          viewBuffer.resize(width, height);
          glViewport(0, 0, width, height);
          viewBuffer.clear();

          printer.render(shader, view, projection, glm::vec3(0.7f, 0.7f, 0.7f),
                         glm::vec3(0.0f, 0.0f, 1.0f));
          model.render(shader, view, projection, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        viewBuffer.unbind();

        ImGui::Image((void *)(intptr_t)viewBuffer.getTexture(),
                     ImGui::GetContentRegionAvail(), ImVec2(0, 1),
                     ImVec2(1, 0));
      }
      ImGui::End();

      ImGui::Begin("Slice View");
      {
        sliceBuffer.bind();
        if (!state.slices.empty()) {
          const int width = ImGui::GetContentRegionAvail().x;
          const int height = ImGui::GetContentRegionAvail().y;

          auto view = topDownCamera.getViewMatrix(printer.getCenter() * ZEROY);
          auto projection = topDownCamera.getProjectionMatrix(width, height);

          sliceBuffer.resize(width, height);
          glViewport(0, 0, width, height);

          sliceBuffer.clear();
          state.slices[state.sliceIndex].render(shader, view, projection);
        }
        sliceBuffer.unbind();

        ImGui::Image((void *)(intptr_t)sliceBuffer.getTexture(),
                     ImGui::GetContentRegionAvail(), ImVec2(0, 1),
                     ImVec2(1, 0));
      }
      ImGui::End();
    }
    ImGui::PopStyleVar();
  });
}