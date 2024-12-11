#pragma once

#include "slice.h"
#include <clipper2/clipper.h>
#include <vector>

struct State {
  using PathsD = Clipper2Lib::PathsD;
  struct {
    float layerHeight = 0.2f;
    int sliceIndex = 1;
    int maxSliceIndex;
    int shellCount = 2;
    float infillDensity = 20.0f;
  } sliceSettings;
  struct {
    bool dropDown = true;
  } objectSettings;
  struct {
    bool showDemoWindow = false;
    glm::ivec2 windowSize{1280, 720};
    bool showSlicePlane = false;
    bool sliceViewFocused = false;
    bool modelViewFocused = false;
    float sliceScale = 5.0f;
  } windowSettings;

  struct {
    int bedTemp = 50;
    int nozzleTemp = 200;
    float nozzleDiameter = 0.4f;
    float printSpeed = 25.0f;
    float infillSpeed = 50.0f;
  } printerSettings;

  struct {
    char inputFile[256] = "../res/models/cube.stl";
    char outputFile[256] = "output.gcode";

  } fileSettings;
  std::vector<Slice> slices;
};

extern State g_state;