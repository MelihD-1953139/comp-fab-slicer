#pragma once

#include "model.h"
#include "slice.h"

#include <clipper2/clipper.core.h>
#include <vector>

inline void rotatePaths(Clipper2Lib::PathsD &paths, float angle) {
  angle = glm::radians(angle);
  std::array<double, 4> rotation{std::cos(angle), -std::sin(angle),
                                 std::sin(angle), std::cos(angle)};

  for (auto &path : paths) {
    for (auto &point : path) {
      const auto x = point.x;
      const auto y = point.y;
      point = {x * rotation[0] + y * rotation[1],
               x * rotation[2] + y * rotation[3]};
    }
  }
}
inline void unRotatePaths(Clipper2Lib::PathsD &paths, float angle) {
  angle = glm::radians(angle);
  std::array<double, 4> rotation{std::cos(angle), -std::sin(angle),
                                 std::sin(angle), std::cos(angle)};

  for (auto &path : paths) {
    for (auto &point : path) {
      const auto x = point.x;
      const auto y = point.y;
      point = {x * rotation[0] + y * rotation[2],
               x * rotation[1] + y * rotation[3]};
    }
  }
}

enum FillType {
  NoFill,
  ConcentricFill,
  LinesFill,
  FillCount,
};

enum InfillType {
  NoInfill,
  LinesInfill,
  Grid,
  Cubic,
  Triangle,
  TriHexagon,
  Tetrahedral,
  QuarterCubic,
  ConcentricInfill,
  HalfTetrahedral,
  InfillCount,
};

enum AdhesionTypes {
  None,
  Brim,
  Skirt,
  Raft,
  AdhesionCount,
};

enum BrimLocation {
  Outside,
  Inside,
  Both,
  BrimLocationCount,
};

class Slicer {
public:
public:
  Slicer() = default;
  Slicer(const char *modelPath);
  void loadModel(const char *modelPath);
  Model &getModel() { return *m_model; };
  void init(float layerHeight, float nozzleDiameter);

  int getLayerCount() const { return m_layerCount; }
  bool hasSlices() const { return m_slices.size() > 0; }

  const Slice &getSlice(size_t index) const { return m_slices.at(index); }

  void createSlices();
  void createWalls(int wallCount);
  void createFill(FillType fillType, int floorCount, int roofCount);
  void createInfill(InfillType infillType, float density);
  void createSupport(float density);

  // Adhesion
  void createBrim(BrimLocation brimLocation, int lineCount);
  void createSkirt(int lineCount, int height, float distance);

  const char *fillTypes[FillType::FillCount]{"None", "Concentric", "Lines"};
  const char *infillTypes[InfillType::InfillCount]{
      "None",        "Lines",
      "Grid",        "Cubic",
      "Triangle",    "Tri-Hexagon",
      "Tetrahedral", "Quarter Cubic",
      "Concentric",  "Half Tetrahedral",
  };

private:
  double
  getShiftOffsetFromInfillOriginAndRotation(const Clipper2Lib::PathsD &area,
                                            const float angle);

  void generateFill(Clipper2Lib::PathsD &fillResult, FillType fillType,
                    const float angle);

  void generateLineInfill(Clipper2Lib::PathsD &infillResult,
                          const double lineDistance, const float angle,
                          float shift);
  void generateGridInfill(Clipper2Lib::PathsD &infillResult,
                          const double lineDistance, const float angle,
                          float shift);

  void generateCubicInfill(Clipper2Lib::PathsD &infillResult,
                           const double lineDistance, const float angle);

  void generateTriangleInfill(Clipper2Lib::PathsD &infillResult,
                              const double lineDistance, const float angle,
                              float shift);
  void generateTriHexagonInfill(Clipper2Lib::PathsD &infillResult,
                                const double lineDistance, const float angle,
                                float shift);
  void generateTetrahedralInfill(Clipper2Lib::PathsD &infillResult,
                                 const double lineDistance);
  void generateQuarterCubicInfill(Clipper2Lib::PathsD &infillResult,
                                  const double lineDistance);
  void generateHalfTetrahedralInfill(Clipper2Lib::PathsD &infillResult,
                                     const double lineDistance,
                                     const float angle, float zShift);
  void generateConcentricInfill(Clipper2Lib::PathsD &infillResult,
                                const double lineDistance);

private:
  std::unique_ptr<Model> m_model;
  std::vector<Slice> m_slices;

  size_t m_layerCount = 0;
  float m_layerHeight;
  float m_lineWidth;

  Clipper2Lib::PathsD m_currentArea;
  size_t m_currentLayer;
  double m_infillLineDistance;
};