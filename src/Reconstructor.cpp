#include "Reconstructor.hpp"
#include "Intersection.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <string>

#ifdef __APPLE__
#include "MetalContext.hpp"
#include "Structs.hpp" // for Match

// Pass params sequentially to avoid any Metal struct padding issues:
// 0..3: fx, fy, cx, cy
// 4..8: k1, k2, p1, p2, k3
// 9..17: r00..r22
// 18..20: ox, oy, oz

struct PointOutput {
  float point[3];
  int valid;
};

void fillCameraParams(const Camera &c, float *p, int offset) {
  cv::Mat mtx = c.getCameraMatrix();
  p[offset + 0] = mtx.at<double>(0, 0);
  p[offset + 1] = mtx.at<double>(1, 1);
  p[offset + 2] = mtx.at<double>(0, 2);
  p[offset + 3] = mtx.at<double>(1, 2);

  cv::Mat dist = c.getDistCoeffs();
  p[offset + 4] = dist.at<double>(0, 0);
  p[offset + 5] = dist.at<double>(0, 1);
  p[offset + 6] = dist.at<double>(0, 2);
  p[offset + 7] = dist.at<double>(0, 3);
  p[offset + 8] = dist.cols > 4 ? dist.at<double>(0, 4) : 0.0f;

  Eigen::Matrix3d r = c.getBasis().transpose();
  p[offset + 9] = r(0, 0);
  p[offset + 10] = r(0, 1);
  p[offset + 11] = r(0, 2);
  p[offset + 12] = r(1, 0);
  p[offset + 13] = r(1, 1);
  p[offset + 14] = r(1, 2);
  p[offset + 15] = r(2, 0);
  p[offset + 16] = r(2, 1);
  p[offset + 17] = r(2, 2);

  Eigen::Vector3d pos = c.getPosition();
  p[offset + 18] = pos.x();
  p[offset + 19] = pos.y();
  p[offset + 20] = pos.z();
}

#endif

Reconstructor::Reconstructor(const std::string &cam_json,
                             const std::string &proj_json)
    : m_camera(cam_json), m_projector(proj_json) {}

void Reconstructor::processMatches(const std::vector<Match> &matches) {
  if (matches.empty())
    return;

  m_pointCloud.resize(matches.size());

#ifdef __APPLE__
  const char *use_cpu = std::getenv("USE_CPU_ONLY");
  if (!use_cpu || std::string(use_cpu) != "1") {
    auto &metalCtx = MetalContext::getInstance();
    if (metalCtx.init()) {
      MTL::Device *device = metalCtx.getDevice();
      MTL::CommandQueue *queue = metalCtx.getCommandQueue();
      MTL::ComputePipelineState *pipeline =
          metalCtx.getRayIntersectionPipeline();

      size_t numMatches = matches.size();

      // Allocate buffers. We use MTL::Buffer's direct initialization over
      // existing memory to avoid looping!
      MTL::Buffer *inBuffer =
          device->newBuffer(matches.data(), numMatches * sizeof(Match),
                            MTL::ResourceStorageModeShared);
      MTL::Buffer *outBuffer = device->newBuffer(
          numMatches * sizeof(PointOutput), MTL::ResourceStorageModeShared);

      // Setup parameters array of 44 floats
      float rParams[44];
      fillCameraParams(m_camera, rParams, 0);     // Fills 0..20
      fillCameraParams(m_projector, rParams, 21); // Fills 21..41
      rParams[42] = m_zMin;
      rParams[43] = m_zMax;

      MTL::Buffer *paramsBuffer = device->newBuffer(
          rParams, sizeof(rParams), MTL::ResourceStorageModeShared);

      // Execute compute shader
      MTL::CommandBuffer *cmdBuf = queue->commandBuffer();
      MTL::ComputeCommandEncoder *encoder = cmdBuf->computeCommandEncoder();

      encoder->setComputePipelineState(pipeline);
      encoder->setBuffer(inBuffer, 0, 0);
      encoder->setBuffer(outBuffer, 0, 1);
      encoder->setBuffer(paramsBuffer, 0, 2);

      MTL::Size gridSize = MTL::Size::Make(numMatches, 1, 1);
      NS::UInteger threadGroupSize = pipeline->maxTotalThreadsPerThreadgroup();
      if (threadGroupSize > numMatches)
        threadGroupSize = numMatches;
      MTL::Size threadgroupSize = MTL::Size::Make(threadGroupSize, 1, 1);

      encoder->dispatchThreads(gridSize, threadgroupSize);
      encoder->endEncoding();
      cmdBuf->commit();
      cmdBuf->waitUntilCompleted();

      PointOutput *outData = (PointOutput *)outBuffer->contents();

      // Read output and build point cloud
      m_pointCloud.clear();
      m_pointCloud.reserve(numMatches);
      for (size_t i = 0; i < numMatches; i++) {
        if (outData[i].valid) {
          m_pointCloud.emplace_back(outData[i].point[0], outData[i].point[1],
                                    outData[i].point[2]);
        }
      }

      inBuffer->release();
      outBuffer->release();
      paramsBuffer->release();

      return;
    } else {
      std::cerr
          << "  [Metal] Failed to initialize/bypassed. Falling back to CPU."
          << std::endl;
    }
  } else {
    std::cout << "  [CPU] Bypassing Metal (USE_CPU_ONLY=1)" << std::endl;
  }
#endif

#pragma omp parallel for
  for (int i = 0; i < (int)matches.size(); ++i) {
    const auto &m = matches[i];
    // 1. Generate 3D Rays from 2D pixel matches
    Eigen::Vector3d ray_c = m_camera.pixelToRay(m.cam_u, m.cam_v);
    Eigen::Vector3d ray_p = m_projector.pixelToRay(m.proj_u, m.proj_v);

    // 2. Perform Triangulation
    Eigen::Vector3d point = intersectRays(m_camera.getPosition(), ray_c,
                                          m_projector.getPosition(), ray_p);

    // 3. Store the resulting 3D point directly into the pre-allocated vector
    m_pointCloud[i] = point;
  }
}

void Reconstructor::saveToPLY(const std::string &output_path) {
  std::ofstream file(output_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open " << output_path << " for writing."
              << std::endl;
    return;
  }

  // Write PLY Header
  file << "ply\n";
  file << "format binary_little_endian 1.0\n";
  file << "element vertex " << m_pointCloud.size() << "\n";
  file << "property float x\n";
  file << "property float y\n";
  file << "property float z\n";
  file << "end_header\n";

  // Write Points in Binary format
  for (const auto &p : m_pointCloud) {
    float x = (float)p.x();
    float y = (float)p.y();
    float z = (float)p.z();
    file.write(reinterpret_cast<const char *>(&x), sizeof(float));
    file.write(reinterpret_cast<const char *>(&y), sizeof(float));
    file.write(reinterpret_cast<const char *>(&z), sizeof(float));
  }

  std::cout << "  Saved " << m_pointCloud.size()
            << " points to binary PLY: " << output_path << std::endl;
}
