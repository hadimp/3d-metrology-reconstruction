#include "Reconstructor.hpp"
#include "Intersection.hpp"
#include <fstream>
#include <iostream>
#include <omp.h>

Reconstructor::Reconstructor(const std::string &cam_json,
                             const std::string &proj_json)
    : m_camera(cam_json), m_projector(proj_json) {}

void Reconstructor::processMatches(const std::vector<Match> &matches) {
  m_pointCloud.resize(matches.size());

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

  // Write PLY Header - we still use ASCII for the header
  file << "ply\n";
  file << "format binary_little_endian 1.0\n";
  file << "element vertex " << m_pointCloud.size() << "\n";
  file << "property float x\n";
  file << "property float y\n";
  file << "property float z\n";
  file << "end_header\n";

  // Write Points in Binary format (much faster than ASCII)
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
