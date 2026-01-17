#pragma once

#include "Camera.hpp"
#include "Structs.hpp"
#include <Eigen/Dense>
#include <vector>

/**
 * Orchestrates the 3D reconstruction process.
 * Loads calibration data, processes 2D-to-3D triangulation, and saves results.
 */
class Reconstructor {
public:
  Reconstructor(const std::string &cam_json, const std::string &proj_json);

  /**
   * Triangulates 3D points from a list of 2D correspondences.
   */
  void processMatches(const std::vector<Match> &matches);

  /**
   * Saves the accumulated point cloud to a PLY file.
   */
  void saveToPLY(const std::string &output_path);

  const std::vector<Eigen::Vector3d> &getPointCloud() const {
    return m_pointCloud;
  }

private:
  Camera m_camera;    // The observing camera
  Camera m_projector; // The structured light source (calibrated as a camera)
  std::vector<Eigen::Vector3d> m_pointCloud;
};
