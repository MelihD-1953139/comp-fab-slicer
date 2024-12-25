#pragma once

#include "slice.h"
#include "slicer.h"
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

    int floorCount = 4;
    int roofCount = 4;

    float retractDistance = 5.0f;
    float minimumRetractDistance = 1.5f;

    bool enableSupport = true;

    FillType fillType = Lines;

    AdhesionTypes adhesionType = Brim;
    BrimLocation brimLocation = Outside;
    int brimLineCount = 20;

    int skirtHeight = 3;
    int skirtLineCount = 3;
    float skirtDistance = 10.0f;

  } sliceSettings;
  struct {
    bool dropDown = true;
  } objectSettings;
  struct {
    bool showDemoWindow = false;
    glm::ivec2 windowSize{1920, 1080};
    bool showSlicePlane = false;
    bool sliceViewFocused = false;
    bool modelViewFocused = false;
    float sliceScale = 5.0f;
  } windowSettings;

  struct {
    int bedTemp = 50;
    int nozzleTemp = 200;
    float nozzleDiameter = 0.4f;
    float printSpeed = 50.0f;
    float infillSpeed = printSpeed;
    float wallSpeed = printSpeed * 0.5;
    float inititalLayerSpeed = printSpeed * 0.2;
  } printerSettings;

  struct {
    char inputFile[256] = "../res/models/cube.stl";
    char outputFile[256] = "output.gcode";

  } fileSettings;

  struct {
    std::vector<PathsD> supportAreas;
    std::vector<Slice> slices;
  } data;
};

extern State g_state;