#pragma once

#include "slice.h"
#include <fstream>

class GcodeWriter {
public:
  static void NewGcodeFile(const char *filename);
  static void WriteHeader(float &extrusion); // TODO add parameters: temperature, speed, etc.
  static void WriteSlice(const Slice &slice, float layerHeight, float nozzle, float &extrusion, bool firstSlice);
  static void WriteFooter(float &extrusion);
  static void CloseGcodeFile();

private:
  static std::ofstream m_file;
};