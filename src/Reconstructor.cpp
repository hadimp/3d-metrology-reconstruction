#include "Reconstructor.hpp"
#include "Intersection.hpp"
#include <iostream>

Reconstructor::Reconstructor(const std::string &cam_json,
                             const std::string &proj_json)
    : m_camera(cam_json), m_projector(proj_json) {}

// Process a list of matches directly
void Reconstructor::processMatches(const std::vector<Match> &matches) {
  int count = 0;
  for (const auto &m : matches) {
    // 1. Generate Rays
    Eigen::Vector3d ray_c = m_camera.pixelToRay(m.cam_u, m.cam_v);
    Eigen::Vector3d ray_p = m_projector.pixelToRay(m.proj_u, m.proj_v);

    // 2. Intersect
    Eigen::Vector3d point = intersectRays(m_camera.getPosition(), ray_c,
                                          m_projector.getPosition(), ray_p);

    // 3. Store
    m_pointCloud.push_back(point);
    count++;
  }
}

void Reconstructor::saveToPLY(const std::string &output_path) {
  std::ofstream file(output_path);
  if (!file.is_open())
    return;

  // Write PLY Header
  file << "ply\n";
  file << "format ascii 1.0\n";
  file << "element vertex " << m_pointCloud.size() << "\n";
  file << "property float x\n";
  file << "property float y\n";
  file << "property float z\n";
  file << "end_header\n";

  // Write Points
  for (const auto &p : m_pointCloud) {
    file << p.x() << " " << p.y() << " " << p.z() << "\n";
  }
}
