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

void usage(const char *program)
{
    Logger::info("Usage: {} <filename>", program);
}

struct State
{
    bool slice;
    bool showDemoWindow;
    std::pair<int, int> windowSize;
    float layerHeight;
    int maxSliceIndex;
    int sliceIndex;
    bool showSlicePlane;
    char fileBuffer[256];
    int shellCount;
    float infillDensity;
    bool dropDown;
    std::vector<Slice> slices;
    std::vector<PathsD> infill;
    std::vector<PathsD> perimeters;
    std::vector<PathsD> shells;
};

PathD generateSparceRectangleInfillV(float density, float nozzleThickness,
                                     PointD min, PointD max)
{
    PathD infill;
    float xLineCount = std::ceil((max.x - min.x) / nozzleThickness);
    float step = xLineCount / density;

    PointD current = {min.x + step, min.y};
    while (current.x <= max.x)
    {
        infill.push_back(current);
        current.y = max.y;
        infill.push_back(current);
        current.x += step;
        infill.push_back(current);
        current.y = min.y;
        infill.push_back(current);
        current.x += step;
    }
    return infill;
}

PathD generateSparceRectangleInfillH(float density, float nozzleThickness,
                                     PointD min, PointD max)
{
    PathD infill;
    float x = min.x;
    float y = min.y;
    float yLineCount = std::ceil((max.y - min.y) / nozzleThickness);
    float step = yLineCount / density;

    PointD current = {x, y + step};
    while (current.y < max.y)
    {
        infill.push_back(current);
        current.x = max.x;
        infill.push_back(current);
        current.y += step;
        infill.push_back(current);
        current.x = min.x;
        infill.push_back(current);
        current.y += step;
    }
    return infill;
}

PathsD generateVerticalOnlyWithoutConnecting(float density, float nozzleThickness, PointD min, PointD max)
{
    float x = min.x;
    float y = min.y;
    float xLineCount = std::ceil((max.x - min.x) / nozzleThickness);
    float step = xLineCount / density;

    PointD start = {x, y};
    PathsD infillLines;
    // Loop to create vertical lines
    while (start.x <= max.x)
    {
        PathD line; // Create a new path for each vertical line
        start.y = min.y;
        line.push_back(start); // Bottom point

        start.y = max.y;
        line.push_back(start); // Top point

        infillLines.push_back(line); // Add to the list of lines

        start.x += step; // Move to the next vertical line
    }
    return infillLines;
}

std::pair<PointD, PointD> getMinMax(PathsD &paths)
{
    PointD min = {std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max()};
    PointD max = {std::numeric_limits<double>::min(),
                  std::numeric_limits<double>::min()};
    for (auto &path : paths)
    {
        for (auto &point : path)
        {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
        }
    }
    return {min, max};
}

int main(int argc, char *argv[])
{
    Logger::setLevel(LogLevel::Trace);

    if (argc < 2)
    {
        Logger::critical("Invalid number of arguments");
        usage(argv[0]);
        return 1;
    }

    auto window = std::unique_ptr<Window>(Window::create(WindowProps("Slicer")));
    window->setVSync(true);

    Shader shader(SHADER_VERT_PATH, SHADER_FRAG_PATH);

    Printer printer("../res/models/plane.obj", "../res/models/plane.obj");
    Model model(argv[1]);
    model.setPosition(printer.getCenter() * ZEROY +
                      glm::vec3(0.0f, model.getHeight() / 2.0f, 0.0f));

    PerspectiveCamera camera(0.0f, 45.0f, 300.0f);
    OrthographicCamera topDownCamera(0.0f, 0.0f, printer.getSize().y);

    State state{
        .slice = false,
        .showDemoWindow = false,
        .windowSize{1280, 720},
        .layerHeight = 0.2f,
        .maxSliceIndex =
            static_cast<int>(std::ceil(model.getHeight() / state.layerHeight)),
        .sliceIndex = 1,
        .showSlicePlane = false,
        .fileBuffer = "",
        .shellCount = 1,
        .infillDensity = 20.0f,
        .dropDown = true,
    };

    strcpy(state.fileBuffer, argv[1]);
    printer.setSliceHeight(state.layerHeight * state.sliceIndex);

    Framebuffer viewBuffer(state.windowSize.first, state.windowSize.second);
    Framebuffer sliceBuffer(printer.getSize().x, printer.getSize().z);

    // Callbacks
    window
        ->onKey([&](int key, int scancode, int action, int mods) -> bool
                {
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
        return false; })
        ->onResize([&](int width, int height) -> bool
                   {
        state.windowSize = {width, height};
        state.slice = true;
        return false; });

    // Main loop
    window->whileOpen([&]()
                      {
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
          model.setPosition(printer.getCenter() * ZEROY);
        }

        ImGui::InputFloat("Printer Nozel", printer.getNozzlePtr(), 0.0f, 0.0f,
                          "%.2f mm");
      }

      if (ImGui::CollapsingHeader("Object settings")) {
        ImGui::InputText("Model file", state.fileBuffer,
                         IM_ARRAYSIZE(state.fileBuffer));
        if (ImGui::Button("Load")) {
          model = Model(state.fileBuffer);
          model.setPosition(printer.getCenter() * ZEROY);
        }

        float position[3] = {model.getPosition().x, model.getPosition().y,
                             model.getPosition().z};
        if (ImGui::SliderFloat3(
                "Position", position, 0,
                glm::min(printer.getSize().x, printer.getSize().z))) {
          model.setPosition({position[0], position[1], position[2]});
        }
        float scale[3] = {model.getScale().x, model.getScale().y,
                          model.getScale().z};
        if (ImGui::SliderFloat3("Scale", scale, 0, 10)) {
          model.setScale({scale[0], scale[1], scale[2]});
        }
        float rotation[3] = {model.getRotation().x, model.getRotation().y,
                             model.getRotation().z};
        if (ImGui::SliderFloat3("Rotation", rotation, -180, 180)) {
          model.setRotation({rotation[0], rotation[1], rotation[2]});
        }

        ImGui::Checkbox("Drop model down", &state.dropDown);
      }

      if (ImGui::CollapsingHeader("Slice settings")) {
        if (ImGui::SliderInt("Slice Index", &state.sliceIndex, 1,
                             state.maxSliceIndex)) {
          printer.setSliceHeight(state.sliceIndex * state.layerHeight);
        }

        ImGui::Checkbox("Show Slice Plane", &state.showSlicePlane);

        if (ImGui::InputFloat("Layer Height", &state.layerHeight, 0.0f, 0.0f,
                              "%.2f mm")) {
          state.layerHeight =
              std::clamp(state.layerHeight, 0.0f, printer.getNozzle() * 0.8f);
          state.maxSliceIndex = static_cast<int>(
              std::ceil(model.getHeight() / state.layerHeight));
        }

        if (ImGui::InputInt("Shell count", &state.shellCount)) {
          state.shellCount = std::clamp(state.shellCount, 1, 10);
        }

        if (ImGui::InputFloat("Infill Density", &state.infillDensity, 0.0f,
                              0.0f, "%.2f %%")) {
          state.infillDensity = std::clamp(state.infillDensity, 0.0f, 100.0f);
        }
      }

      if (ImGui::Button("Slice", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {

        state.slices.clear();
        state.infill.clear();
        state.shells.clear();
        state.perimeters.clear();

        for (int i = 0; i < state.maxSliceIndex; ++i) {
          auto slice = model.getSlice(state.layerHeight * i + 0.000000001);
          auto perimeter = Union(slice, FillRule::EvenOdd);
          perimeter = InflatePaths(perimeter, -printer.getNozzle() / 2.0f,
                                   JoinType::Miter, EndType::Polygon);

          PathsD lastShell = perimeter;
          PathsD shells_pathds;
          for (int j = 0; j < state.shellCount - 1; ++j) {
            auto shells = InflatePaths(perimeter, -printer.getNozzle() * j,
                                       JoinType::Miter, EndType::Polygon);
            for (auto &shell : shells)
              shells_pathds.push_back(shell);

            if (j == state.shellCount - 2)
              lastShell = shells;
          }

          auto toIntersect = InflatePaths(lastShell, -printer.getNozzle(),
                                          JoinType::Miter, EndType::Polygon);
          auto [min, max] = getMinMax(toIntersect);

          PathD gridLines;
          if (i % 2 == 0)
            gridLines = generateSparceRectangleInfillH(
                state.infillDensity, printer.getNozzle(), min, max);
          else
            gridLines = generateSparceRectangleInfillV(
                state.infillDensity, printer.getNozzle(), min, max);
          




            if( i == 1 ){
            for (int j = 0; j < gridLines.size(); j++)
            {
                std::cout << gridLines[j] << std::endl;
            }
            std::cout << "end of gridlines" << std::endl;
          }
          
        //   PathsD gridlinesMelih = generateVerticalOnlyWithoutConnecting(state.infillDensity, printer.getNozzle(), min, max);
        //   if( i == 1 ){
        //     for (int j = 0; j < gridlinesMelih.size(); j++)
        //     {
        //         std::cout << gridlinesMelih[j] << std::endl;
        //     }
        //   }
        if( i == 1 ){
            for (int j = 0; j < toIntersect.size(); j++)
            {
                std::cout << toIntersect[j] << std::endl;
            }
            std::cout << "end of toIntersect" << std::endl;
          }




          auto infill = Intersect({gridLines}, toIntersect, FillRule::EvenOdd);
          if( i == 1 ){
            for (int j = 0; j < infill.size(); j++)
            {
                std::cout << infill[j] << std::endl;
            }
            std::cout << "end of infill" << std::endl;
          }
          state.shells.push_back(shells_pathds);
          state.perimeters.push_back(perimeter);
          state.infill.push_back(infill);

          state.slices.emplace_back(state.shells[i], state.infill[i], state.perimeters[i]);

        }
      }

      if (ImGui::Button("Export to g-code",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        GcodeWriter::NewGcodeFile("output.gcode");
        GcodeWriter::WriteHeader();
        for( int i = 0; i < state.slices.size(); i++){
            GcodeWriter::WriteSlice(state.slices[i], state.layerHeight * i,
                                printer.getNozzle());
        }
        
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
                         glm::vec3(0.0f, 0.0f, 1.0f), state.showSlicePlane);
          if (state.dropDown) {
            auto pos = model.getPosition();
            model.setPosition({pos.x, model.getHeight() / 2, pos.z});
          }
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
    ImGui::PopStyleVar(); });
}