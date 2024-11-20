#include "gcodeWriter.h"

#define FA 1.75f

std::ofstream GcodeWriter::m_file;

void GcodeWriter::NewGcodeFile(const char *filename) { m_file.open(filename); }

void GcodeWriter::WriteHeader()
{
    m_file << "M140 S60\n";
    m_file << "M190 S60\n";
    m_file << "M104 S200\n";
    m_file << "M109 S200\n";
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
    m_file << "G92 E0 ;zero the extruder\n";
}

void GcodeWriter::WriteSlice(const Slice &slice, float layerHeight,
                             float nozzle)
{
    // Write shells
    m_file << "; Writing shells\n";
    for (const Contour &shell : slice.getShells())
    {
        auto points = shell.getPoints();
        if (points.empty())
            continue;

        m_file << "G0 X" << points[0].x << " Y" << points[0].z << " Z" << layerHeight << "\n";

        for (size_t i = 1; i < points.size(); i++)
        {
            float extrusion = layerHeight * nozzle * glm::distance(points[i - 1], points[i]) / FA;
            m_file << "G1 X" << points[i].x << " Y" << points[i].z << " E" << extrusion << "\n";
        }
        // Aanzetten als men terugwilt naar beginpunt.
        // m_file << "G1 X" << points[0].x << " Y" << points[0].z << "\n";
    }

    // Write infill
    m_file << "; Writing infill\n";
    for (const Contour &infill : slice.getInfill())
    {
        auto points = infill.getPoints();
        if (points.empty())
            continue;

        m_file << "G0 X" << points[0].x << " Y" << points[0].z << " Z" << layerHeight << "\n";

        for (size_t i = 1; i < points.size(); i++)
        {
            float extrusion = layerHeight * nozzle * glm::distance(points[i - 1], points[i]) / FA;
            m_file << "G1 X" << points[i].x << " Y" << points[i].z << " E" << extrusion << "\n";
        }

        // Aanzetten als men terugwilt naar beginpunt.
        // m_file << "G1 X" << points[0].x << " Y" << points[0].z << "\n";
    }

    // Write perimeter
    m_file << "; Writing perimeter\n";
    for (const Contour &perimeter : slice.getPerimeters())
    {
        auto points = perimeter.getPoints();
        if (points.empty())
            continue;

        m_file << "G0 X" << points[0].x << " Y" << points[0].z << " Z" << layerHeight << "\n";

        for (size_t i = 1; i < points.size(); i++)
        {
            float extrusion = layerHeight * nozzle * glm::distance(points[i - 1], points[i]) / FA;
            m_file << "G1 X" << points[i].x << " Y" << points[i].z << " E" << extrusion << "\n";
        }

        // Aanzetten als men terugwilt naar beginpunt.
        // m_file << "G1 X" << points[0].x << " Y" << points[0].z << "\n";
    }
}

void GcodeWriter::WriteFooter()
{
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
