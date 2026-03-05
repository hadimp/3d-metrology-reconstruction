#include "Reconstructor.hpp"
#include "Intersection.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <string>

#ifdef __APPLE__
#include "MetalContext.hpp"
#endif

Reconstructor::Reconstructor(const std::string &cam_json,
                             const std::string &proj_json)
    : m_camera(cam_json), m_projector(proj_json), m_zMin(0.0), m_zMax(1000.0) {}

void Reconstructor::processMatches(const std::vector<Match> &matches) {
  if (matches.empty())
    return;

#ifdef __APPLE__
  const char *use_cpu = std::getenv("USE_CPU_ONLY");
  if (!use_cpu || std::string(use_cpu) != "1") {
    if (MetalContext::getInstance().executeRayIntersection(
            matches, m_camera, m_projector, (float)m_zMin, (float)m_zMax,
            m_pointCloud)) {
      return;
    }
    std::cerr << "  [Metal] Execution failed. Falling back to CPU."
              << std::endl;
  }
#endif

  processMatchesCPU(matches);
}

void Reconstructor::processMatchesCPU(const std::vector<Match> &matches) {
  int max_threads = omp_get_max_threads();
  std::vector<std::vector<Eigen::Vector3d>> thread_local_points(max_threads);

#pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    auto &local_points = thread_local_points[thread_id];

#pragma omp for
    for (int i = 0; i < (int)matches.size(); ++i) {
      const auto &m = matches[i];
      // 1. Generate 3D Rays from 2D pixel matches
      Eigen::Vector3d ray_c = m_camera.pixelToRay(m.cam_u, m.cam_v);
      Eigen::Vector3d ray_p = m_projector.pixelToRay(m.proj_u, m.proj_v);

      // 2. Perform Triangulation
      Eigen::Vector3d point = intersectRays(m_camera.getPosition(), ray_c,
                                            m_projector.getPosition(), ray_p);

      // 3. Filter by depth (parity with GPU)
      if (point.z() >= m_zMin && point.z() <= m_zMax) {
        local_points.push_back(point);
      }
    }
  }

  // Merge the points
  size_t total_points = 0;
  for (const auto &bucket : thread_local_points) {
    total_points += bucket.size();
  }
  m_pointCloud.clear();
  m_pointCloud.reserve(total_points);
  for (const auto &bucket : thread_local_points) {
    m_pointCloud.insert(m_pointCloud.end(), bucket.begin(), bucket.end());
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
