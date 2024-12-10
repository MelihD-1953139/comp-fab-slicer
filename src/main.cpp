#include "camera.h"
#include "framebuffer.h"
#include "gcodeWriter.h"
#include "printer.h"
#include "resources.h"
#include "state.h"

#include <Nexus.h>
#include <Nexus/Log.h>
#include <Nexus/Window/GLFWWindow.h>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>

#define ZEROY glm::vec3(1.0f, 0.0f, 1.0f)
#define SHADER_VERT_PATH "../res/shaders/base.vert"
#define SHADER_FRAG_PATH "../res/shaders/base.frag"

using namespace Nexus;
using namespace Clipper2Lib;

State g_state{};

void usage(const char *program) {
  Logger::info("Usage: {} <filename>", program);
}

PathsD generateSparseRectangleInfill(float density, PointD min, PointD max) {
  double step = 1.0f / density;
  bool leftToRight = true;

  PathsD infill;
  for (double y = min.y; y <= max.y; y += step) {
    leftToRight ? infill.push_back({
                      {min.x, y},
                      {max.x, y},
                  })
                : infill.push_back({
                      {max.x, y},
                      {min.x, y},
                  });
    leftToRight = !leftToRight;
  }
  for (double x = min.x; x <= max.x; x += step) {
    leftToRight ? infill.push_back({
                      {x, min.y},
                      {x, max.y},
                  })
                : infill.push_back({
                      {x, max.y},
                      {x, min.y},
                  });
    leftToRight = !leftToRight;
  }

  return infill;
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
    strcpy(g_state.fileBuffer, argv[1]);

  auto window = std::unique_ptr<Window>(Window::create(WindowProps("Slicer")));
  window->setVSync(true);

  Shader previewShader(baseVertexShader, baseFragmentShader);
  Shader sliceShader(sliceVertexShader, sliceFragmentShader);

  Printer printer;
  Model model(g_state.fileBuffer);
  model.setPosition(printer.getCenter() * ZEROY +
                    glm::vec3(0.0f, model.getHeight() / 2.0f, 0.0f));
  g_state.maxSliceIndex =
      std::ceil<int>(model.getHeight() / g_state.layerHeight);

  PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
  OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

  printer.setSliceHeight(g_state.layerHeight * g_state.sliceIndex);

  Framebuffer viewBuffer(g_state.windowSize.x, g_state.windowSize.y);
  Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

  // Callbacks
  window
      ->onKey([&](int key, int scancode, int action, int mods) -> bool {
        if (action == GLFW_RELEASE)
          return false;

        if (g_state.modelViewFocused) {
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
        } else if (g_state.sliceViewFocused) {
          switch (key) {
          case GLFW_KEY_W:
          case GLFW_KEY_UP:
          case GLFW_KEY_KP_ADD:
            g_state.sliceScale += 0.5f;
            break;
          case GLFW_KEY_S:
          case GLFW_KEY_DOWN:
          case GLFW_KEY_KP_SUBTRACT:
            g_state.sliceScale -= 0.5f;
            break;
          }
        }
        return false;
      })
      ->onResize([&](int width, int height) -> bool {
        g_state.windowSize = {width, height};
        return false;
      });

  // Main loop
  window->whileOpen([&]() {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (g_state.showDemoWindow)
      ImGui::ShowDemoWindow();

    ImGui::Begin("Control Panel");
    {
      if (ImGui::CollapsingHeader("Printer settings")) {
        if (ImGui::InputInt3("Printer Size (mm)", printer.getSizePtr()))
          model.setPosition(printer.getCenter() * ZEROY);

        ImGui::InputFloat("Printer Nozel", printer.getNozzlePtr(), 0.0f, 0.0f,
                          "%.2f mm");

        ImGui::InputInt("Bed Temp", &g_state.bedTemp);
        ImGui::InputInt("Nozzle Temp", &g_state.nozzleTemp);
        ImGui::InputFloat("Speed", &g_state.speed, 0.0f, 0.0f, "%.2f mm/s");
      }

      if (ImGui::CollapsingHeader("Object settings")) {
        g_state.maxSliceIndex =
            std::ceil<int>(model.getHeight() / g_state.layerHeight);

        ImGui::InputText("Model file", g_state.fileBuffer,
                         IM_ARRAYSIZE(g_state.fileBuffer));
        if (ImGui::Button("Load")) {
          model = Model(g_state.fileBuffer);
          model.setPosition(printer.getCenter() * ZEROY);
          g_state.maxSliceIndex = static_cast<int>(
              std::ceil(model.getHeight() / g_state.layerHeight));
        }

        ImGui::DragFloat3("Position", model.getPositionPtr(), 0,
                          glm::min(printer.getSize().x, printer.getSize().z));
        ImGui::DragFloat3("Scale", model.getScalePtr(), 0, 10);
        ImGui::DragFloat3("Rotation", model.getRotationPtr(), -180, 180);

        ImGui::Checkbox("Drop model down", &g_state.dropDown);
      }

      if (ImGui::CollapsingHeader("Slice settings")) {
        if (ImGui::SliderInt("Slice Index", &g_state.sliceIndex, 1,
                             g_state.maxSliceIndex - 1)) {
          printer.setSliceHeight(g_state.sliceIndex * g_state.layerHeight);
        }

        ImGui::Checkbox("Show Slice Plane", &g_state.showSlicePlane);

        if (ImGui::InputFloat("Layer Height", &g_state.layerHeight, 0.0f, 0.0f,
                              "%.2f mm")) {
          g_state.layerHeight =
              std::clamp(g_state.layerHeight, 0.0f, printer.getNozzle() * 0.8f);
        }

        if (ImGui::InputInt("Shell count", &g_state.shellCount)) {
          g_state.shellCount = std::clamp(g_state.shellCount, 1, 10);
        }

        if (ImGui::InputFloat("Infill Density", &g_state.infillDensity, 0.0f,
                              0.0f, "%.2f %%")) {
          g_state.infillDensity =
              std::clamp(g_state.infillDensity, 0.0f, 100.0f);
        }
      }

      if (ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        g_state.slices.clear();
        for (int i = 1; i < g_state.maxSliceIndex - 1; ++i) {
          auto slice = model.getSlice(g_state.layerHeight * i);
          auto perimeter = Union(slice, FillRule::EvenOdd);

          PathsD shells;
          PathsD lastShell;
          for (int j = 0; j < g_state.shellCount; ++j) {
            lastShell = InflatePaths(perimeter,
                                     -printer.getNozzle() / 2.0f -
                                         printer.getNozzle() * j,
                                     JoinType::Miter, EndType::Polygon);
            shells.append_range(lastShell);
          }

          PathsD infillRaw = generateSparseRectangleInfill(
              g_state.infillDensity / 100.0f, {0, 0},
              {printer.getSize().x, printer.getSize().z});

          ClipperD clipper;
          clipper.AddClip(lastShell.empty() ? perimeter : lastShell);
          clipper.AddOpenSubject(infillRaw);
          PathsD infillClosed, infillOpen;
          clipper.Execute(ClipType::Intersection, FillRule::EvenOdd,
                          infillClosed, infillOpen);

          g_state.slices.emplace_back(shells, infillOpen);
        }
      }

      if (ImGui::Button("Export to g-code",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        GcodeWriter::WriteGcode(g_state.slices,
                                {
                                    .outFilePath = "output.gcode",
                                    .layerHeight = g_state.layerHeight,
                                    .nozzle = printer.getNozzle(),
                                    .bedTemp = g_state.bedTemp,
                                    .nozzleTemp = g_state.nozzleTemp,
                                    .speed = g_state.speed,
                                });
      }
    }
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    {
      ImGui::Begin("3D View");
      {
        g_state.modelViewFocused = ImGui::IsWindowFocused();

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
                         glm::vec3(0.0f, 0.0f, 1.0f), g_state.showSlicePlane);
          if (g_state.dropDown) {
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
        g_state.sliceViewFocused = ImGui::IsWindowFocused();

        sliceBuffer.bind();
        if (!g_state.slices.empty()) {
          const int width = ImGui::GetContentRegionAvail().x;
          const int height = ImGui::GetContentRegionAvail().y;

          sliceShader.use();

          auto position = printer.getCenter() * ZEROY;
          auto view = topDownCamera.getViewMatrix(position);
          auto projection = topDownCamera.getProjectionMatrix(width, height);

          sliceShader.setMat4("view", view);
          sliceShader.setMat4("projection", projection);

          sliceBuffer.resize(width, height);
          sliceBuffer.clear();
          sliceShader.setBool("useShading", false);

          g_state.slices[g_state.sliceIndex].render(sliceShader, position,
                                                    g_state.sliceScale);
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