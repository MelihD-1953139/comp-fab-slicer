#pragma once

#include "clipper2/clipper.core.h"
#include "slice.h"
#include "slicer.h"
#include <fstream>
#include <vector>

#define FILLAMENT_DIAMETER 1.75f

class GcodeWriter {
public:
  GcodeWriter(const char *filename, const Slicer &slicer);

private:
  void NewGcodeFile(const char *filename);
  void WriteHeader();
  void WriteSlice(const Slice &slice);
  void WritePaths(const Clipper2Lib::PathsD &paths, float speed);
  void WritePath(const Clipper2Lib::PathD &path, float speed);
  void WriteFooter();
  void CloseGcodeFile();

  std::ofstream m_file;
  float extrusion;
  float layerHeight;
  Clipper2Lib::PointD currentPosition;

  constexpr static const float fa =
      FILLAMENT_DIAMETER * FILLAMENT_DIAMETER * glm::pi<float>() / 4;
};