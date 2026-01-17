#pragma once

#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>

using json = nlohmann::json;

/**
 * Represents a pinhole camera or projector with lens distortion.
 * Handles coordinate transformations between 2D pixels and 3D rays.
 */
class Camera {
public:
  Camera(const std::string &calibration_file);

  Eigen::Vector3d pixelToRay(double u, double v);

  cv::Mat getCameraMatrix() const { return m_cameraMatrix; }
  Eigen::Vector3d getPosition() const;

private:
  cv::Mat m_cameraMatrix; // 3x3 Intrinsic matrix [fx 0 cx; 0 fy cy; 0 0 1]
  cv::Mat m_distCoeffs;   // Lens distortion coefficients
  int m_width, m_height;

  Eigen::Matrix3d m_basis;  // Rotation basis for world coordinates
  Eigen::Vector3d m_origin; // Camera center in world coordinates
};
