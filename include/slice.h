#pragma once

#include "shader.h"

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#define GREEN glm::vec3(0.0f, 1.0f, 0.0f)
#define YELLOW glm::vec3(1.0f, 1.0f, 0.0f)
#define BLUE glm::vec3(0.0f, 0.0f, 1.0f)
#define BLUEGREEN glm::vec3(0.0f, 1.0f, 1.0f)
#define RED glm::vec3(1.0f, 0.0f, 0.0f)
#define ORANGE glm::vec3(1.0f, 0.5f, 0.0f)

struct Line {
  Clipper2Lib::PointD p1, p2;
  void setNextPoint(Clipper2Lib::PointD point);
  bool operator==(const Line &other) const;

private:
  bool firstPointSet = false;
};

class Slice {
  using PathsD = Clipper2Lib::PathsD;
  using PathD = Clipper2Lib::PathD;

  struct PathData {
    std::vector<PathsD> paths;
    std::vector<uint> VAOs;
  };

  enum PathType {
    InnerWall,
    OuterWall,
    Skin,
    Infill,
    Support,
  };

public:
  Slice() = default;
  Slice(std::vector<Line> &lineSegments);
  std::pair<glm::vec2, glm::vec2> getBounds() const;

  void render(Shader &shader, const glm::vec3 &position,
              const float &scale) const;

  void clear();

  // assumes the shell is closed
  void addOuterWall(const PathsD &wall);
  void addInnerWall(const PathsD &shell);
  void addFill(const PathsD &fill);
  void setFillArea(const PathsD &fillArea) { m_fillArea = fillArea; }
  void addInfill(const PathsD &infill);
  void addSupport(const PathsD &support);
  void setSupportArea(const PathsD supportArea) { m_supportArea = supportArea; }

  bool hasPerimeter() const { return m_paths.contains(OuterWall); }
  bool hasWalls() const { return m_paths.contains(InnerWall); }
  bool hasFill() const { return m_paths.contains(Skin); }
  bool hasInfill() const { return m_paths.contains(Infill); }
  bool hasSupport() const { return m_paths.contains(Support); }

  const PathsD &getPerimeter() const;
  const PathD getOuterMostPerimeter() const;
  const std::vector<PathsD> &getShells() const;
  const PathsD &getInnermostShell() const;
  const std::vector<PathsD> &getFill() const;
  const PathsD &getFillArea() const { return m_fillArea; }
  const std::vector<PathsD> &getInfill() const;
  const std::vector<PathsD> &getSupport() const;
  void removeSupport();
  const PathsD &getSupportArea() const { return m_supportArea; }

private:
  const double EPSILON = 1e-3;

  std::unordered_map<PathType, PathData> m_paths;

  PathsD m_supportArea;
  PathsD m_fillArea;

private:
  void initOpenGLBuffers(std::vector<uint> &VAOs, const PathsD &paths);
  uint initOpenGLBuffer(const PathD &path);
  void drawPaths(const PathData &pd, Shader &shader, glm::vec3 color) const;
  void drawPath(const PathD &path, uint vaoIndex) const;
};