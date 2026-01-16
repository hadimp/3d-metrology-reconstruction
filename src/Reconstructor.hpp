#pragma once

#include "Camera.hpp"
#include "Structs.hpp"
#include <Eigen/Dense>
#include <string>
#include <vector>

class Reconstructor {
public:
  Reconstructor(const std::string &cam_json, const std::string &proj_json);

  // Reads a CSV file where each line is: cam_u, cam_v, proj_u, proj_v
  void processMatches(const std::string &csv_path);

  // Process a list of matches directly
  void processMatches(const std::vector<Match> &matches);

  // Saves the computed point cloud to a standard PLY file
  void saveToPLY(const std::string &output_path);

  const std::vector<Eigen::Vector3d> &getPointCloud() const {
    return m_pointCloud;
  }

private:
  Camera m_camera;
  Camera m_projector;
  std::vector<Eigen::Vector3d> m_pointCloud;
};
