#include "camera.h"
#include "framebuffer.h"
#include "gcodeWriter.h"
#include "printer.h"
#include "resources.h"
#include "slicer.h"
#include "state.h"

#include <Nexus.h>
#include <Nexus/Log.h>
#include <Nexus/Window/GLFWWindow.h>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>
#include <cmath>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>

#define ZEROY glm::vec3(1.0f, 0.0f, 1.0f)

using namespace Nexus;
using namespace Clipper2Lib;

State g_state{};

void usage(const char *program) {
  Logger::info("Usage: {} <filename>", program);
}

void printMatrix(const glm::mat4 &matrix) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      std::cout << matrix[j][i] << " ";
    }
    std::cout << std::endl;
  }
}

int main(int argc, char *argv[]) {
  Logger::setLevel(LogLevel::Trace);

  if (argc >= 2)
    strcpy(g_state.fileSettings.inputFile, argv[1]);

  auto window = std::unique_ptr<Window>(
      Window::create(WindowProps("Slicer", g_state.windowSettings.windowSize.x,
                                 g_state.windowSettings.windowSize.y)));
  window->setVSync(true);

  Shader previewShader(baseVertexShader, baseFragmentShader);
  Shader sliceShader(sliceVertexShader, sliceFragmentShader);

  Printer printer;
  Slicer slicer(g_state.fileSettings.inputFile);
  slicer.init(g_state.sliceSettings.layerHeight);
  Model &model = slicer.getModel();

  model.setPosition(printer.getCenter() * ZEROY +
                    glm::vec3(0.0f, model.getHeight() / 2.0f, 0.0f));
  g_state.sliceSettings.maxSliceIndex = slicer.getLayerCount();

  PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
  OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

  printer.setSliceHeight(g_state.sliceSettings.layerHeight *
                         g_state.sliceSettings.sliceIndex);

  Framebuffer viewBuffer(g_state.windowSettings.windowSize.x,
                         g_state.windowSettings.windowSize.y);
  Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

  // Callbacks
  window
      ->onKey([&](int key, int scancode, int action, int mods) -> bool {
        if (action == GLFW_RELEASE)
          return false;

        if (g_state.windowSettings.modelViewFocused) {
          switch (key) {
          case GLFW_KEY_W:
            camera.orbit(0.0f, -1.0f);
            break;
          case GLFW_KEY_S:
            camera.orbit(0.0f, 1.0f);
            break;
          case GLFW_KEY_A:
            camera.orbit(1.0f, 0.0f);
            break;
          case GLFW_KEY_D:
            camera.orbit(-1.0f, 0.0f);
            break;
          case GLFW_KEY_UP:
            camera.offsetDistanceFromTarget(-5.0f);
            break;
          case GLFW_KEY_DOWN:
            camera.offsetDistanceFromTarget(5.0f);
            break;
          }
        } else if (g_state.windowSettings.sliceViewFocused) {
          switch (key) {
          case GLFW_KEY_W:
          case GLFW_KEY_UP:
          case GLFW_KEY_KP_ADD:
            g_state.windowSettings.sliceScale += 0.5f;
            break;
          case GLFW_KEY_S:
          case GLFW_KEY_DOWN:
          case GLFW_KEY_KP_SUBTRACT:
            g_state.windowSettings.sliceScale -= 0.5f;
            break;
          }
        }
        return false;
      })
      ->onResize([&](int width, int height) -> bool {
        g_state.windowSettings.windowSize = {width, height};
        return false;
      });

  // Main loop
  window->whileOpen([&]() {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    ImGui::Begin("Control Panel");
    {
      if (ImGui::CollapsingHeader("Printer settings")) {
        if (ImGui::InputInt3("Printer Size (mm)", printer.getSizePtr()))
          model.setPosition(printer.getCenter() * ZEROY);

        ImGui::InputFloat("Printer Nozel", printer.getNozzlePtr(), 0.0f, 0.0f,
                          "%.2f mm");

        ImGui::InputInt("Bed Temp", &g_state.printerSettings.bedTemp);
        ImGui::InputInt("Nozzle Temp", &g_state.printerSettings.nozzleTemp);
        ImGui::InputFloat("Speed", &g_state.printerSettings.printSpeed, 0.0f,
                          0.0f, "%.1f mm/s");
      }

      if (ImGui::CollapsingHeader("Model settings")) {
        g_state.sliceSettings.maxSliceIndex = std::ceil<int>(
            model.getHeight() / g_state.sliceSettings.layerHeight);

        ImGui::InputText("Model file", g_state.fileSettings.inputFile,
                         IM_ARRAYSIZE(g_state.fileSettings.inputFile));
        if (ImGui::Button("Load")) {
          model = Model(g_state.fileSettings.inputFile);
          model.setPosition(printer.getCenter() * ZEROY);
          g_state.sliceSettings.maxSliceIndex = static_cast<int>(
              std::ceil(model.getHeight() / g_state.sliceSettings.layerHeight));
          g_state.sliceSettings.sliceIndex = 1;
          g_state.data.slices.clear();
          g_state.data.supportAreas.clear();
        }

        ImGui::DragFloat3("Position", model.getPositionPtr(), 0,
                          std::min(printer.getSize().x, printer.getSize().z));
        ImGui::DragFloat3("Scale", model.getScalePtr(), 0, 10);
        ImGui::DragFloat3("Rotation", model.getRotationPtr(), -180, 180);

        ImGui::Checkbox("Drop model down", &g_state.objectSettings.dropDown);
      }

      if (ImGui::CollapsingHeader("Slice settings")) {
        if (ImGui::SliderInt("Slice Index", &g_state.sliceSettings.sliceIndex,
                             1, slicer.getLayerCount())) {
          printer.setSliceHeight((g_state.sliceSettings.sliceIndex - 1) *
                                 g_state.sliceSettings.layerHeight);
        }

        ImGui::Checkbox("Show Slice Plane",
                        &g_state.windowSettings.showSlicePlane);

        if (ImGui::InputFloat("Layer Height",
                              &g_state.sliceSettings.layerHeight, 0.0f, 0.0f,
                              "%.2f mm")) {
          g_state.sliceSettings.layerHeight =
              std::clamp(g_state.sliceSettings.layerHeight, 0.0f,
                         printer.getNozzle() * 0.8f);
        }

        if (ImGui::InputInt("Shell count", &g_state.sliceSettings.shellCount)) {
          g_state.sliceSettings.shellCount =
              std::clamp(g_state.sliceSettings.shellCount, 1, 10);
        }

        ImGui::InputInt("Floor count", &g_state.sliceSettings.floorCount);
        ImGui::InputInt("Roof count", &g_state.sliceSettings.roofCount);

        if (ImGui::InputFloat("Infill Density",
                              &g_state.sliceSettings.infillDensity, 0.0f, 0.0f,
                              "%.2f %%")) {
          g_state.sliceSettings.infillDensity =
              std::clamp(g_state.sliceSettings.infillDensity, 0.0f, 100.0f);
        }
        ImGui::Checkbox("Enable support", &g_state.sliceSettings.enableSupport);
        const char *adhesionTypes[] = {"None", "Brim", "Skirt", "Raft"};
        ImGui::Combo(
            "Build plate adhesion",
            reinterpret_cast<int *>(&g_state.sliceSettings.adhesionType),
            adhesionTypes, IM_ARRAYSIZE(adhesionTypes));
        switch (g_state.sliceSettings.adhesionType) {
        case Brim: {
          ImGui::InputInt("Brim line count",
                          &g_state.sliceSettings.brimLineCount);
          const char *brimLocations[] = {"Outside only", "Inside only",
                                         "Both sides"};
          ImGui::Combo(
              "Brim location",
              reinterpret_cast<int *>(&g_state.sliceSettings.brimLocation),
              brimLocations, IM_ARRAYSIZE(brimLocations));
          break;
        }
        case Skirt: {
          ImGui::InputInt("Skirt lineCount",
                          &g_state.sliceSettings.skirtLineCount);
          ImGui::InputInt("Skirt height", &g_state.sliceSettings.skirtHeight);
          ImGui::InputFloat("Skirt distance",
                            &g_state.sliceSettings.skirtDistance, 0.0f, 0.0f,
                            "%.1f mm");
        }
        default:
          break;
        }
      }

      if (ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        Logger::info("Creating slices");
        slicer.createSlices(g_state.sliceSettings.layerHeight);

        Logger::info("Creating walls");
        slicer.createWalls(g_state.sliceSettings.shellCount,
                           g_state.printerSettings.nozzleDiameter);

        Logger::info("Creating fill and infill");
        slicer.createFillAndInfill(
            g_state.printerSettings.nozzleDiameter,
            g_state.sliceSettings.floorCount, g_state.sliceSettings.roofCount,
            g_state.sliceSettings.infillDensity / 100.0f, printer.getSize());

        if (g_state.sliceSettings.enableSupport) {
          Logger::info("Creating support");
          slicer.createSupport(g_state.printerSettings.nozzleDiameter,
                               g_state.sliceSettings.infillDensity / 100.0f);
        }

        switch (g_state.sliceSettings.adhesionType) {
        case AdhesionTypes::Brim:
          slicer.createBrim(g_state.sliceSettings.brimLocation,
                            g_state.sliceSettings.brimLineCount,
                            g_state.printerSettings.nozzleDiameter);
          break;
        case Skirt:
          slicer.createSkirt(g_state.sliceSettings.skirtLineCount,
                             g_state.sliceSettings.skirtHeight,
                             g_state.sliceSettings.skirtDistance,
                             g_state.printerSettings.nozzleDiameter);
          break;
        default:
          break;
        }
        Logger::info("Slicing complete");
      }

      if (ImGui::Button("Export to g-code",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        GcodeWriter writer(g_state.fileSettings.outputFile,
                           g_state.data.slices);
      }
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    {
      ImGui::Begin("3D View");
      {
        g_state.windowSettings.modelViewFocused = ImGui::IsWindowFocused();

        const int width = ImGui::GetContentRegionAvail().x;
        const int height = ImGui::GetContentRegionAvail().y;
        auto view = camera.getViewMatrix(printer.getCenter() * ZEROY);
        auto projection = camera.getProjectionMatrix(width, height);

        viewBuffer.bind();
        {
          viewBuffer.resize(width, height);
          viewBuffer.clear();
          previewShader.use();
          previewShader.setVec3("lightPos", camera.getPosition());
          previewShader.setBool("useShading", false);
          printer.render(previewShader, view, projection,
                         glm::vec3(0.7f, 0.7f, 0.7f),
                         glm::vec3(0.0f, 0.0f, 1.0f),
                         g_state.windowSettings.showSlicePlane);
          if (g_state.objectSettings.dropDown) {
            auto pos = model.getPosition();
            model.setPosition({pos.x, model.getHeight() / 2, pos.z});
          }
          previewShader.setBool("useShading", true);
          model.render(previewShader, view, projection,
                       glm::vec3(1.0f, 0.0f, 0.0f));
        }
        viewBuffer.unbind();

        ImGui::Image((void *)(intptr_t)viewBuffer.getTexture(),
                     ImGui::GetContentRegionAvail(), ImVec2(0, 1),
                     ImVec2(1, 0));
      }
      ImGui::End();

      ImGui::Begin("Slice View");
      {
        g_state.windowSettings.sliceViewFocused = ImGui::IsWindowFocused();

        sliceBuffer.bind();
        // if (!g_state.data.slices.empty()) {
        if (slicer.hasSlices()) {
          const int width = ImGui::GetContentRegionAvail().x;
          const int height = ImGui::GetContentRegionAvail().y;

          sliceShader.use();

          auto position = printer.getCenter() * ZEROY;
          auto view = topDownCamera.getViewMatrix(position);
          auto projection = topDownCamera.getProjectionMatrix(width, height);

          sliceShader.setMat4("view", view);
          sliceShader.setMat4("projection", projection);

          sliceBuffer.resize(width, height);
          sliceBuffer.clear(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
          sliceShader.setBool("useShading", false);

          // g_state.data.slices[g_state.sliceSettings.sliceIndex - 1].render(
          //     sliceShader, position, g_state.windowSettings.sliceScale);

          slicer.getSlice(g_state.sliceSettings.sliceIndex - 1)
              .render(sliceShader, position, g_state.windowSettings.sliceScale);
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