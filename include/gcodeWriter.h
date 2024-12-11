#pragma once

#include "clipper2/clipper.core.h"
#include "slice.h"
#include <fstream>

#define FILLAMENT_DIAMETER 1.75f

struct GcodeSettings {
  const char *outFilePath;
  float layerHeight;
  float nozzle;
  int bedTemp;
  int nozzleTemp;
  float speed;
};

class GcodeWriter {
public:
  static void WriteGcode(const std::vector<Slice> &slices);

private:
  static void NewGcodeFile(const char *filename);
  static void WriteHeader();
  static void WriteSlice(const Slice &slice);
  static void WritePaths(const Clipper2Lib::PathsD &paths, float speed);
  static void WritePath(const Clipper2Lib::PathD &path, float speed);
  static void WriteFooter();
  static void CloseGcodeFile();

  static std::ofstream m_file;
  static float extrusion;
  static float layerHeight;

  constexpr static const float fa =
      FILLAMENT_DIAMETER * FILLAMENT_DIAMETER * glm::pi<float>() / 4;
};