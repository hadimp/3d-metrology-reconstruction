#pragma once
#include "Camera.hpp"
#include "Structs.hpp"
#include <Eigen/Dense>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <vector>

struct PointOutput {
  float point[3];
  int valid;
};

class MetalContext {
public:
  static MetalContext &getInstance();

  MTL::Device *getDevice() const { return m_device; }
  MTL::CommandQueue *getCommandQueue() const { return m_commandQueue; }
  MTL::ComputePipelineState *getRayIntersectionPipeline() const {
    return m_rayIntersectPipeline;
  }

  bool init();
  void cleanup();

  /**
   * Executes the ray intersection kernel on the GPU.
   * Maps camera/projector parameters and match data to GPU buffers.
   */
  bool executeRayIntersection(const std::vector<Match> &matches,
                              const Camera &camera, const Camera &projector,
                              float zMin, float zMax,
                              std::vector<Eigen::Vector3d> &outPoints);

private:
  MetalContext();
  ~MetalContext();

  MetalContext(const MetalContext &) = delete;
  MetalContext &operator=(const MetalContext &) = delete;

  void fillCameraParams(const Camera &c, float *p, int offset);

  MTL::Device *m_device;
  MTL::CommandQueue *m_commandQueue;
  MTL::Library *m_library;
  MTL::ComputePipelineState *m_rayIntersectPipeline;
};
