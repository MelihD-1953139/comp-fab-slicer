#pragma once

#include "model.h"
#include "slice.h"

#include <clipper2/clipper.core.h>
#include <cstdint>
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
inline void rotatePaths(Clipper2Lib::Paths64 &paths, float angle) {
  using namespace Clipper2Lib;
  angle = glm::radians(angle);
  std::array<double, 4> rotation{std::cos(angle), -std::sin(angle),
                                 std::sin(angle), std::cos(angle)};

  for (Path64 &path : paths) {
    for (Point64 &point : path) {
      const double x = static_cast<double>(point.x);
      const double y = static_cast<double>(point.y);
      point = {std::llrint(x * rotation[0] + y * rotation[1]),
               std::llrint(x * rotation[2] + y * rotation[3])};
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
inline void unRotatePaths(Clipper2Lib::Paths64 &paths, float angle) {
  using namespace Clipper2Lib;
  angle = glm::radians(angle);
  std::array<double, 4> rotation{std::cos(angle), -std::sin(angle),
                                 std::sin(angle), std::cos(angle)};

  for (Path64 &path : paths) {
    for (Point64 &point : path) {
      const double x = static_cast<double>(point.x);
      const double y = static_cast<double>(point.y);
      point = {std::llrint(x * rotation[0] + y * rotation[2]),
               std::llrint(x * rotation[1] + y * rotation[3])};
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
  GridInfill,
  Cubic,
  Triangle,
  TriHexagon,
  Tetrahedral,
  QuarterCubic,
  ConcentricInfill,
  InfillCount,
};

enum SupportType {
  NoSupport,
  LinesSupport,
  GridSupport,
  Triangles,
  ConcentricSupport,
  SupportCount
};

enum AdhesionTypes {
  NoAdhesion,
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
  using Paths64 = Clipper2Lib::Paths64;

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
  void createSupport(SupportType supportType, float density, size_t wallCount,
                     size_t brimWallCount);

  // Adhesion
  void createBrim(BrimLocation brimLocation, int lineCount);
  void createSkirt(int lineCount, int height, float distance);

  const char *fillTypes[FillType::FillCount]{"None", "Concentric", "Lines"};
  const char *infillTypes[InfillType::InfillCount]{
      "None",        "Lines",       "Grid",          "Cubic",      "Triangle",
      "Tri-Hexagon", "Tetrahedral", "Quarter Cubic", "Concentric",
  };
  const char *supportTypes[SupportType::SupportCount]{
      "None", "Lines", "Grid", "Triangles", "Concentric"};

  int64_t extraShift;

private:
  int64_t getLineDistance(uint lineCount, float density) const;

  int64_t getShiftOffsetFromInfillOriginAndRotation(const Paths64 &area,
                                                    const double angle);

  void generateFill(Paths64 &fillResult, FillType fillType, const double angle);

  void generateLineInfill(Paths64 &infillResult, const int64_t lineDistance,
                          const double angle, int64_t shift);
  void generateGridInfill(Paths64 &infillResult, const int64_t lineDistance,
                          const double angle);

  void generateCubicInfill(Paths64 &infillResult, const int64_t lineDistance,
                           const double angle);

  void generateTriangleInfill(Paths64 &infillResult, const int64_t lineDistance,
                              const double angle);
  void generateTriHexagonInfill(Paths64 &infillResult,
                                const int64_t lineDistance, const double angle);
  void generateTetrahedralInfill(Paths64 &infillResult,
                                 const int64_t lineDistance);
  void generateQuarterCubicInfill(Paths64 &infillResult,
                                  const int64_t lineDistance);
  void generateHalfTetrahedralInfill(Paths64 &infillResult,
                                     const int64_t lineDistance,
                                     const double angle, const double zShift);
  void generateConcentricInfill(Paths64 &infillResult,
                                const int64_t lineDistance);

private:
  std::unique_ptr<Model> m_model;
  std::vector<Slice> m_slices;

  size_t m_layerCount = 0;
  float m_layerHeight;
  int64_t m_lineWidth;
  int64_t m_shift;

  Paths64 m_currentArea;
  size_t m_currentLayer;
  double m_infillLineDistance;
};