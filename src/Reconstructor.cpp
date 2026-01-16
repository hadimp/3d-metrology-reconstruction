#include "Reconstructor.hpp"
#include "Intersection.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Reconstructor::Reconstructor(const std::string &cam_json,
                             const std::string &proj_json)
    : m_camera(cam_json), m_projector(proj_json) {
  std::cout << "Reconstructor initialized." << std::endl;
}

void Reconstructor::processMatches(const std::string &csv_path) {
  std::ifstream file(csv_path);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open match file: " << csv_path << std::endl;
    return;
  }

  std::string line;
  int count = 0;
  while (std::getline(file, line)) {
    // Skip empty lines or comments
    if (line.empty() || line[0] == '#')
      continue;

    std::stringstream ss(line);
    std::string segment;
    std::vector<double> values;

    // Split by comma
    while (std::getline(ss, segment, ',')) {
      values.push_back(std::stod(segment));
    }

    if (values.size() >= 4) {
      double u_cam = values[0];
      double v_cam = values[1];
      double u_proj = values[2];
      double v_proj = values[3];

      // 1. Generate Rays
      // Note: Python implementation adds 1.0 (0.5 for pixel center + 0.5 for
      // Mitsuba convention) to the projector coordinates.
      Eigen::Vector3d ray_c = m_camera.pixelToRay(u_cam, v_cam);
      Eigen::Vector3d ray_p =
          m_projector.pixelToRay(u_proj + 1.0, v_proj + 1.0);

      // 2. Intersect
      Eigen::Vector3d point = intersectRays(m_camera.getPosition(), ray_c,
                                            m_projector.getPosition(), ray_p);

      // 3. Store
      m_pointCloud.push_back(point);
      count++;
    }
  }
  std::cout << "Processed " << count << " matches (CSV)." << std::endl;
}

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
  std::cout << "Reconstructed " << count << " points from matches."
            << std::endl;
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

  std::cout << "Saved point cloud to " << output_path << std::endl;
}
