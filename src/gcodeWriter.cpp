#include "gcodeWriter.h"

std::ofstream GcodeWriter::m_file;
float GcodeWriter::extrusion;
float GcodeWriter::layerHeight;
float GcodeWriter::layerThickness;
float GcodeWriter::nozzle;
int GcodeWriter::bedTemp;
int GcodeWriter::nozzleTemp;
int GcodeWriter::speed;

void GcodeWriter::WriteGcode(const std::vector<Slice> &slices,
                             GcodeSettings settings) {
  GcodeWriter::extrusion = 0;
  GcodeWriter::layerHeight = settings.layerHeight;
  GcodeWriter::nozzle = settings.nozzle;
  GcodeWriter::bedTemp = settings.bedTemp;
  GcodeWriter::nozzleTemp = settings.nozzleTemp;
  GcodeWriter::layerThickness = settings.layerHeight;
  GcodeWriter::speed = settings.speed * 60;

  NewGcodeFile(settings.outFilePath);
  WriteHeader();
  m_file << "M107 ;turn off fan\n";
  m_file << ";LAYER_COUNT:" << slices.size() << "\n";
  for (int i = 0; i < slices.size(); i++) {
    m_file << ";LAYER:" << i << "\n";
    layerHeight = settings.layerHeight * (i + 1);
    if (i == 2)
      m_file << "M106 S255 ;turn on fan\n";
    WriteSlice(slices[i]);
  }
  WriteFooter();
  CloseGcodeFile();
}

void GcodeWriter::NewGcodeFile(const char *filename) { m_file.open(filename); }

void GcodeWriter::WriteHeader() {
  m_file << "M140 S" << GcodeWriter::bedTemp << "\n";
  m_file << "M190 S" << GcodeWriter::bedTemp << "\n";
  m_file << "M104 S" << GcodeWriter::nozzleTemp << "\n";
  m_file << "M109 S" << GcodeWriter::nozzleTemp << "\n";
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

void GcodeWriter::WritePath(const Contour &contour) {
  auto points = contour.getPoints();
  if (points.empty())
    return;

  m_file << "G0 F6000 X" << points[0].x << " Y" << points[0].z << " Z"
         << layerHeight << "\n";
  for (size_t i = 1; i < points.size(); i++) {
    extrusion +=
        layerThickness * nozzle * glm::distance(points[i - 1], points[i]) / fa;
    m_file << "G1 F" << speed << " X" << points[i].x << " Y" << points[i].z
           << " E" << extrusion << "\n";
  }
}

void GcodeWriter::WriteSlice(const Slice &slice) {
  m_file << ";TYPE:WALL-INNER\n";
  for (const Contour &shell : slice.getShells())
    WritePath(shell);

  m_file << ";TYPE:WALL-OUTER\n";
  for (const Contour &perimeter : slice.getPerimeters())
    WritePath(perimeter);

  m_file << ";TYPE:FILL\n";
  for (const Contour &infill : slice.getInfill())
    WritePath(infill);
}

void GcodeWriter::WriteFooter() {
  m_file << "G1 F1800 E-3 ; retract filament\n";
  m_file << "G1 F3000 Z20 ; lift nozzle\n";
  m_file << "G1 X0 Y235 F1000 ; move to cooling position\n";
  m_file << "M104 S0 ; turn off temperature\n";
  m_file << "M140 S0 ; turn off heatbed\n";
  m_file << "M107 ; turn off fan\n";
  m_file << "M220 S100 ; set speed factor back to 100%\n";
  m_file << "M221 S100 ; set extrusion factor back to 100%\n";
  m_file << "G91 ; set to relative positioning\n";
  m_file << "G90 ; set to absolute positioning\n";
  m_file << "M84 ; disable motors\n";
  m_file << "M82 ; set extruder to absolute mode\n";
}

void GcodeWriter::CloseGcodeFile() { m_file.close(); }
