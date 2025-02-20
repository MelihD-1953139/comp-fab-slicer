#include "gcodeWriter.h"
#include "state.h"
#include "utils.h"

#include <clipper2/clipper.core.h>
#include <iomanip>
#include <ios>

GcodeWriter::GcodeWriter(const char *filepath, const Slicer &slicer) {
  extrusion = 0;
  layerHeight = g_state.sliceSettings.layerHeight;

  float infillspeed = g_state.printerSettings.infillSpeed;
  float wallspeed = g_state.printerSettings.wallSpeed;
  g_state.printerSettings.infillSpeed =
      g_state.printerSettings.inititalLayerSpeed;
  g_state.printerSettings.wallSpeed =
      g_state.printerSettings.inititalLayerSpeed;

  NewGcodeFile("output.gcode");
  WriteHeader();
  m_file << "M107 ;turn off fan\n";
  m_file << ";LAYER_COUNT:" << slicer.getLayerCount() << "\n";
  for (int i = 0; i < slicer.getLayerCount(); i++) {
    m_file << ";LAYER:" << i << "\n";
    layerHeight = g_state.sliceSettings.layerHeight * (i + 1);
    if (i == 2) {
      m_file << "M106 S255 ;turn on fan\n";
      g_state.printerSettings.wallSpeed = wallspeed;
      g_state.printerSettings.infillSpeed = infillspeed;
    }
    WriteSlice(slicer.getSlice(i));
  }
  WriteFooter();
  CloseGcodeFile();
}

void GcodeWriter::NewGcodeFile(const char *filename) { m_file.open(filename); }

void GcodeWriter::WriteHeader() {
  m_file << "M140 S" << g_state.printerSettings.bedTemp << "\n";
  m_file << "M190 S" << g_state.printerSettings.bedTemp << "\n";
  m_file << "M104 S" << g_state.printerSettings.nozzleTemp << "\n";
  m_file << "M109 S" << g_state.printerSettings.nozzleTemp << "\n";
  m_file << "G21 ;set units to millimeters\n";
  m_file << "M82 ;set extruder to absolute mode\n";
  m_file << "G28 ;home all axes\n";
  m_file << "G92 E0 ;zero the extruder\n";
  m_file << "G1 Z2.0 F3000\n";
  m_file << "G1 X0.1 Y20 Z0.3 F5000.0 ;move to start-line position\n";
  m_file << "G1 X0.1 Y200.0 Z0.3 F1500.0 E15 ;draw 1st line\n";
  m_file << "G1 X0.4 Y200.0 Z0.3 F5000.0 ;move to side a little\n";
  m_file << "G1 X0.4 Y20 Z0.3 F1500.0 E30 ;draw 2nd line\n";
  m_file << "G92 E0 ;zero the extruder\n";
  m_file
      << "G1 Z2.0 F3000 ;move Z up little to prevent scratching of Heat Bed\n";
  m_file << "G1 X5 Y20 Z0.3 F5000.0 ; Move over to prevent blob squish\n";
  m_file << "G92 E0 ;zero the extruder\n";
  m_file << "G1 F2700 E-5\n";
}

void GcodeWriter::WritePaths(const Clipper2Lib::PathsD &paths, float speed) {
  for (auto &path : paths)
    WritePath(path, speed);
}

void GcodeWriter::WritePath(const Clipper2Lib::PathD &path, float speed) {
  if (path.empty())
    return;

  if (distance(currentPosition, path[0]) >
      g_state.sliceSettings.minimumRetractDistance) {
    // Retract
    m_file << "G1 F1800 E" << extrusion - g_state.sliceSettings.retractDistance
           << " ; retract filament\n";
    // Move
    m_file << "G0 F6000 X" << std::fixed << std::setprecision(3) << path[0].x
           << " Y" << path[0].y << " Z" << layerHeight << "\n";
    // Unretract
    m_file << "G1 F1800 E" << extrusion << " ; unretract filament\n";
  } else {
    // Move
    m_file << "G0 F6000 X" << std::fixed << std::setprecision(3) << path[0].x
           << " Y" << path[0].y << " Z" << layerHeight << "\n";
  }

  currentPosition = path[0];
  for (size_t i = 1; i < path.size(); i++) {
    float dist = distance(path[i - 1], path[i]);
    currentPosition = path[i];
    extrusion += g_state.sliceSettings.layerHeight *
                 g_state.printerSettings.nozzleDiameter * dist / fa;
    m_file << "G1 F" << std::fixed << std::setprecision(0) << speed
           << std::setprecision(3) << " X" << path[i].x << " Y" << path[i].y
           << std::setprecision(5) << " E" << extrusion << "\n";
  }
}

void GcodeWriter::WriteSlice(const Slice &slice) {

  if (slice.hasSupport()) {
    m_file << ";TYPE:SUPPORT\n";
    for (auto &paths : slice.getSupport())
      WritePaths(paths, g_state.printerSettings.wallSpeed * 60.0f);
  }

  if (slice.hasWalls()) {
    m_file << ";TYPE:WALL-INNER\n";
    auto shells = slice.getShells();
    for (auto it = shells.rbegin(); it != shells.rend(); ++it) {
      WritePaths(*it, g_state.printerSettings.wallSpeed * 60.0f);
    }
  }

  if (slice.hasPerimeter()) {
    m_file << ";TYPE:WALL-OUTER\n";
    WritePaths(slice.getPerimeter(), g_state.printerSettings.wallSpeed * 60.0f);
  }

  if (slice.hasFill()) {
    m_file << ";TYPE:SKIN\n";
    for (auto &skin : slice.getFill())
      WritePaths(skin, g_state.printerSettings.infillSpeed * 60.0f);
  }

  if (slice.hasInfill()) {
    m_file << ";TYPE:FILL\n";
    for (auto &infill : slice.getInfill())
      WritePaths(infill, g_state.printerSettings.infillSpeed * 60.0f);
  }
}

void GcodeWriter::WriteFooter() {
  m_file << "G91 ;Relative positioning\n";
  m_file << "G1 E-2 F2700 ;Retract a bit\n";
  m_file << "G1 E-2 Z0.2 F2400 ;Retract and raise Z\n";
  m_file << "G1 X5 Y5 F3000 ;Wipe out\n";
  m_file << "G1 Z10 ;Raise Z more\n";
  m_file << "G90 ;Absolute positioning\n";

  m_file << "G1 X0 Y{machine_depth} ;Present print\n";
  m_file << "M106 S0 ;Turn-off fan\n";
  m_file << "M104 S0 ;Turn-off hotend\n";
  m_file << "M140 S0 ;Turn-off bed\n";

  m_file << "M84 X Y E ;Disable all steppers but Z\n";
}

void GcodeWriter::CloseGcodeFile() { m_file.close(); }
