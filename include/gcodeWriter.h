#pragma once

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
  static void WriteGcode(const std::vector<Slice> &slices,
                         const GcodeSettings settings);

private:
  static void NewGcodeFile(const char *filename);
  static void WriteHeader();
  static void WriteSlice(const Slice &slice);
  static void WritePath(const Contour &contour);
  static void WriteFooter();
  static void CloseGcodeFile();

  static std::ofstream m_file;
  static float extrusion;
  static float layerHeight;
  static float layerThickness;
  static float nozzle;
  static int bedTemp;
  static int nozzleTemp;
  static int speed;
  constexpr static const float fa =
      FILLAMENT_DIAMETER * FILLAMENT_DIAMETER * glm::pi<float>() / 4;
};