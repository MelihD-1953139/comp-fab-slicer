#include "camera.h"
#include "framebuffer.h"
#include "gcodeWriter.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "printer.h"
#include "resources.h"
#include "state.h"
#include "utils.h"

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

void rotatePaths(PathsD &paths, float angle) {
  auto [minx, miny, maxx, maxy] = GetBounds(paths);
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(
      model, glm::vec3((minx + maxx) / 2.0f, 0.0f, (miny + maxy) / 2.0f));
  model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::translate(
      model, -glm::vec3((minx + maxx) / 2.0f, 0.0f, (miny + maxy) / 2.0f));
  for (auto &path : paths) {
    for (auto &point : path) {
      glm::vec4 p = {point.x, 0.0f, point.y, 1.0f};
      p = model * p;
      point.x = p.x;
      point.y = p.z;
    }
  }
}

PathsD generateConcentricFill(float nozzleDiameter, const PathsD &lastShell) {
  std::vector<PathsD> fills{closePathsD(InflatePaths(
      lastShell, -nozzleDiameter, JoinType::Miter, EndType::Polygon))};
  while (fills.back().size() > 0) {
    fills.push_back(closePathsD(InflatePaths(
        fills.back(), -nozzleDiameter, JoinType::Miter, EndType::Polygon)));
  }

  PathsD fill;
  for (auto &paths : fills)
    fill.append_range(paths);

  return fill;
}

PathsD generateSparseRectangleInfill(float density, PointD min, PointD max,
                                     float angle = 45.0f) {
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

  if (angle != 0.0f)
    rotatePaths(infill, angle);
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
    strcpy(g_state.fileSettings.inputFile, argv[1]);

  auto window = std::unique_ptr<Window>(
      Window::create(WindowProps("Slicer", g_state.windowSettings.windowSize.x,
                                 g_state.windowSettings.windowSize.y)));
  window->setVSync(true);

  Shader previewShader(baseVertexShader, baseFragmentShader);
  Shader sliceShader(sliceVertexShader, sliceFragmentShader);

  Printer printer;
  Model model(g_state.fileSettings.inputFile);
  model.setPosition(printer.getCenter() * ZEROY +
                    glm::vec3(0.0f, model.getHeight() / 2.0f, 0.0f));
  g_state.sliceSettings.maxSliceIndex =
      std::ceil<int>(model.getHeight() / g_state.sliceSettings.layerHeight);

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

    if (g_state.windowSettings.showDemoWindow)
      ImGui::ShowDemoWindow();

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

      if (ImGui::CollapsingHeader("Object settings")) {
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
        }

        ImGui::DragFloat3("Position", model.getPositionPtr(), 0,
                          glm::min(printer.getSize().x, printer.getSize().z));
        ImGui::DragFloat3("Scale", model.getScalePtr(), 0, 10);
        ImGui::DragFloat3("Rotation", model.getRotationPtr(), -180, 180);

        ImGui::Checkbox("Drop model down", &g_state.objectSettings.dropDown);
      }

      if (ImGui::CollapsingHeader("Slice settings")) {
        if (ImGui::SliderInt("Slice Index", &g_state.sliceSettings.sliceIndex,
                             1, g_state.sliceSettings.maxSliceIndex)) {
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
      }

      if (ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        g_state.data.slices.clear();
        g_state.data.slices.resize(g_state.sliceSettings.maxSliceIndex);

        auto layerHeight = g_state.sliceSettings.layerHeight;
        for (int i = 0; i < g_state.sliceSettings.maxSliceIndex; ++i) {
          auto slice = model.getSlice(layerHeight / 2.0f + layerHeight * i);
          PathsD perimeter = slice.getShells().front();

          PathsD lastShell;
          for (int j = 0; j < g_state.sliceSettings.shellCount; ++j) {
            lastShell = InflatePaths(perimeter,
                                     -printer.getNozzle() / 2.0f -
                                         printer.getNozzle() * j,
                                     JoinType::Miter, EndType::Polygon);
            g_state.data.slices[i].addShell(closePathsD(lastShell));
          }
        }

        for (int i = 0; i < g_state.sliceSettings.maxSliceIndex; ++i) {
          std::vector<PathsD> infill;
          // first g_state.sliceSettings.floorCount layers are always solid
          if (i < g_state.sliceSettings.floorCount ||
              i >= g_state.sliceSettings.maxSliceIndex -
                       g_state.sliceSettings.roofCount) {
            PathsD fill = generateConcentricFill(
                g_state.printerSettings.nozzleDiameter,
                g_state.data.slices[i].getShells().back());
            g_state.data.slices[i].addFill(fill);
            continue;
          }

          // Since we know we are not in the first n layers, we can assume there
          // are always n layers below the current one

          // Find floor sections
          ClipperD clipper;
          clipper.AddClip(g_state.data.slices[i - 1].getInnermostShell());
          for (int j = 2; j <= g_state.sliceSettings.floorCount; ++j) {
            clipper.AddSubject(g_state.data.slices[i - j].getInnermostShell());
          }
          PathsD floor;
          clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, floor);
          floor = Difference(g_state.data.slices[i].getInnermostShell(), floor,
                             FillRule::EvenOdd);
          g_state.data.slices[i].addFill(generateConcentricFill(
              g_state.printerSettings.nozzleDiameter, floor));

          // Find roof sections
          clipper.Clear();
          clipper.AddClip(g_state.data.slices[i + 1].getInnermostShell());
          for (int j = 2; j <= g_state.sliceSettings.roofCount; ++j) {
            clipper.AddSubject(g_state.data.slices[i + j].getInnermostShell());
          }
          PathsD roof;
          clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, roof);
          roof = Difference(g_state.data.slices[i].getInnermostShell(), roof,
                            FillRule::EvenOdd);
          g_state.data.slices[i].addFill(generateConcentricFill(
              g_state.printerSettings.nozzleDiameter, roof));

          // Find infill sections
          auto section = Difference(g_state.data.slices[i].getInnermostShell(),
                                    Union(floor, roof, FillRule::EvenOdd),
                                    FillRule::EvenOdd);

          PathsD infillRaw = generateSparseRectangleInfill(
              g_state.sliceSettings.infillDensity / 100.0f, {0.0f, 0.0f},
              {printer.getSize().x, printer.getSize().z});

          clipper.Clear();
          clipper.AddClip(section);
          clipper.AddOpenSubject(infillRaw);
          PathsD infillClosed, infillOpen;
          clipper.Execute(ClipType::Intersection, FillRule::EvenOdd,
                          infillClosed, infillOpen);

          infill.push_back(infillOpen);
          for (auto &fill : infill)
            g_state.data.slices[i].addInfill(fill);
        }

        for (int i = g_state.sliceSettings.maxSliceIndex - 1; i >= 0; --i) {
          if (i == g_state.sliceSettings.maxSliceIndex - 1) {
            g_state.data.slices[i].addSupport(PathsD());
            g_state.data.supportAreas.emplace_back();
            continue;
          }

          ClipperD clipper;
          clipper.AddSubject(g_state.data.slices[i + 1].getPerimeter());
          clipper.AddSubject(g_state.data.supportAreas.front());
          PathsD previousPerimeterAndSupport;
          clipper.Execute(ClipType::Union, FillRule::EvenOdd,
                          previousPerimeterAndSupport);

          float a = std::min(g_state.printerSettings.nozzleDiameter / 2.0f,
                             layerHeight);
          PathsD dilatedPerimeters =
              InflatePaths(g_state.data.slices[i].getPerimeter(), a,
                           JoinType::Miter, EndType::Polygon);

          auto supportArea = Difference(previousPerimeterAndSupport,
                                        dilatedPerimeters, FillRule::EvenOdd);
          g_state.data.supportAreas.insert(g_state.data.supportAreas.begin(),
                                           supportArea);

          PathsD supportInfill =
              i < g_state.sliceSettings.floorCount
                  ? generateConcentricFill(
                        g_state.printerSettings.nozzleDiameter, supportArea)
                  : generateSparseRectangleInfill(
                        g_state.sliceSettings.infillDensity / 100.0f,
                        {0.0f, 0.0f},
                        {printer.getSize().x, printer.getSize().z}, 0.0f);

          clipper.Clear();
          clipper.AddClip(supportArea);
          clipper.AddOpenSubject(supportInfill);
          PathsD supportAreaOpen, supportAreaClosed;
          clipper.Execute(ClipType::Intersection, FillRule::EvenOdd,
                          supportAreaClosed, supportAreaOpen);

          g_state.data.slices[i].addSupport(supportAreaOpen);
        }
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
        if (!g_state.data.slices.empty()) {
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

          g_state.data.slices[g_state.sliceSettings.sliceIndex - 1].render(
              sliceShader, position, g_state.windowSettings.sliceScale);
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