#include "slicer.h"
#include "Nexus/Log.h"
#include "clipper2/clipper.core.h"
#include "clipper2/clipper.engine.h"
#include "clipper2/clipper.h"
#include "clipper2/clipper.offset.h"
#include "glm/ext/vector_int2.hpp"
#include "model.h"
#include "state.h"
#include "utils.h"

#include <algorithm>
#include <hfs/hfs_format.h>
#include <memory>

using namespace Clipper2Lib;

Slicer::Slicer(const char *modelPath)
    : m_model(std::make_unique<Model>(modelPath)) {}

void Slicer::loadModel(const char *modelPath) {
  m_model = std::make_unique<Model>(modelPath);
  m_slices.clear();
}

void Slicer::init(float layerHeight) {
  m_layerCount = m_model->getLayerCount(layerHeight);
}

void Slicer::createSlices(float layerHeight) {
  m_layerCount = m_model->getLayerCount(layerHeight);
  m_slices.clear();

  for (size_t i = 0; i < m_layerCount; ++i) {
    auto sliceHeight = layerHeight / 2.0f + layerHeight * i + 1e-15;
    m_slices.push_back(m_model->getSlice(sliceHeight));
  }
}

void Slicer::createWalls(int wallCount, float nozzleDiameter) {
  for (size_t i = 0; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    auto objectPerimeter = slice.getPerimeter();
    slice.clear();

    for (size_t j = 0; j < wallCount; ++j) {
      float delta = -nozzleDiameter / 2.0f - nozzleDiameter * j;
      PathsD wall = InflatePaths(objectPerimeter, delta, JoinType::Round,
                                 EndType::Polygon);
      slice.addShell(closePathsD(wall));
    }
  }
}

void Slicer::createFillAndInfill(float nozzleDiameter, int floorCount,
                                 int roofCount, float infillDensity,
                                 const glm::ivec2 &printerSize) {
  // First `floorCount` layers are always filled
  for (size_t i = 0; i < floorCount; ++i) {
    auto &slice = m_slices[i];
    auto fill =
        generateConcentricFill(nozzleDiameter, slice.getInnermostShell());
    slice.addFill(fill);
  }

  for (size_t i = floorCount; i < m_layerCount - roofCount; ++i) {
    auto &slice = m_slices[i];
    // Find floor sections;

    PathsD floorArea = m_slices[i - 1].getInnermostShell();
    for (size_t j = 2; j <= floorCount; ++j) {
      floorArea = Intersect(floorArea, m_slices[i - j].getInnermostShell(),
                            FillRule::EvenOdd);
    }
    floorArea =
        Difference(slice.getInnermostShell(), floorArea, FillRule::EvenOdd);
    PathsD floor = generateConcentricFill(nozzleDiameter, floorArea);

    PathsD roofArea = m_slices[i + 1].getInnermostShell();
    for (size_t j = 2; j <= roofCount; ++j) {
      roofArea = Intersect(roofArea, m_slices[i + j].getInnermostShell(),
                           FillRule::EvenOdd);
    }
    roofArea =
        Difference(slice.getInnermostShell(), roofArea, FillRule::EvenOdd);
    PathsD roof = generateConcentricFill(nozzleDiameter, roofArea);

    PathsD infillArea = Difference(
        slice.getInnermostShell(),
        Union(floorArea, roofArea, FillRule::EvenOdd), FillRule::EvenOdd);
    RectD printerSizeRect = {0.0, 0.0, (double)printerSize.x,
                             (double)printerSize.y};
    PathsD infill =
        generateSparseRectangleInfill(infillDensity, printerSizeRect);

    ClipperD clipper;
    clipper.AddClip(infillArea);
    clipper.AddOpenSubject(infill);

    PathsD discard;
    clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard, infill);

    slice.addFill(floor);
    slice.addFill(roof);
    slice.addInfill(infill);
  }

  // Last `roofCount` layers are always filled
  for (size_t i = m_layerCount - roofCount; i < m_layerCount; ++i) {
    auto &slice = m_slices[i];
    auto fill =
        generateConcentricFill(nozzleDiameter, slice.getInnermostShell());
    slice.addFill(fill);
  }
}

void Slicer::createSupport(float nozzleDiameter, float density) {
  for (auto it = m_slices.rbegin() + 1; it < m_slices.rend(); ++it) {
    auto previousSliceIT = it - 1;
    auto prevPerimAndSupport =
        Union(previousSliceIT->getPerimeter(),
              previousSliceIT->getSupportArea(), FillRule::EvenOdd);

    auto dilatedPerimeter =
        InflatePaths(it->getPerimeter(), nozzleDiameter * 3.0f, JoinType::Round,
                     EndType::Polygon);

    auto supportArea =
        Difference(prevPerimAndSupport, dilatedPerimeter, FillRule::EvenOdd);

    auto lastLayerSupport = Difference(previousSliceIT->getPerimeter(),
                                       it->getPerimeter(), FillRule::EvenOdd);

    supportArea = Difference(supportArea, lastLayerSupport, FillRule::EvenOdd);

    it->setSupportArea(supportArea);

    if (it == m_slices.rend() - 1) {
      auto support = generateConcentricFill(nozzleDiameter, supportArea);
      it->addSupport(closePathsD(support));
    } else {
      ;
      auto support =
          generateSparseRectangleInfill(density, GetBounds(supportArea), 0.0f);
      ClipperD clipper;
      clipper.AddClip(supportArea);
      clipper.AddOpenSubject(support);
      PathsD discard;
      clipper.Execute(ClipType::Intersection, FillRule::EvenOdd, discard,
                      support);
      it->addSupport(support);
    }
  }
}

void Slicer::createBrim(BrimLocation brimLocation, int lineCount,
                        float lineWidth) {
  auto &slice = m_slices.front();
  const PathsD &perimeter = slice.getPerimeter();

  for (int i = 1; i <= lineCount; ++i) {

    PathsD brim = InflatePaths(perimeter, lineWidth * i, JoinType::Round,
                               EndType::Polygon);

    auto it = std::remove_if(brim.begin(), brim.end(),
                             [&](const PathD &path) -> bool {
                               switch (brimLocation) {
                               case Outside:
                                 return !IsPositive(path);
                               case Inside:
                                 return IsPositive(path);
                               case Both:
                                 return false;
                               }
                             });
    brim.erase(it, brim.end());

    slice.addSupport(closePathsD(brim));
  }
}