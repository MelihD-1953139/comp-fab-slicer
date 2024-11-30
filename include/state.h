#pragma once

#include "slice.h"
#include <clipper2/clipper.h>
#include <vector>

struct State {
  using PathsD = Clipper2Lib::PathsD;
  bool showDemoWindow = false;
  glm::ivec2 windowSize{1280, 720};
  float layerHeight = 0.2f;
  int sliceIndex = 1;
  int maxSliceIndex;
  bool showSlicePlane = false;
  char fileBuffer[256] = "../res/models/cube.stl";
  int shellCount = 2;
  float infillDensity = 20.0f;
  bool dropDown = true;
  int bedTemp = 50;
  int nozzleTemp = 200;
  float speed = 50.0f;
  bool sliceViewFocused = false;
  bool modelViewFocused = false;
  float sliceScale = 5.0f;
  std::vector<Slice> slices;
  std::vector<PathsD> infill;
  std::vector<PathsD> perimeters;
  std::vector<PathsD> shells;
};

extern State g_state;