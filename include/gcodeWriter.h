#pragma once

#include "slice.h"
#include <fstream>

class GcodeWriter {
public:
  static void NewGcodeFile(const char *filename);
  static void WriteHeader(); // TODO add parameters: temperature, speed, etc.
  static void WriteSlice(const Slice &slice, float layerHeight, float nozzle);
  static void WriteFooter();
  static void CloseGcodeFile();

private:
  static std::ofstream m_file;
};