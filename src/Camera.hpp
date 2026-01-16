#pragma once

#include <Eigen/Dense>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include <string>

using json = nlohmann::json;

class Camera {
public:
  // Parsing constructor
  Camera(const std::string &calibration_file);

  // Core functionality: Turn a 2D pixel (u,v) into a 3D ray direction (x,y,z)
  // The result is in WORLD COORDINATES.
  Eigen::Vector3d pixelToRay(double u, double v);

  // Getters for debugging
  cv::Mat getCameraMatrix() const { return m_cameraMatrix; }
  Eigen::Vector3d getPosition() const;

private:
  // Intrinsics (OpenCV)
  cv::Mat m_cameraMatrix;
  cv::Mat m_distCoeffs;
  int m_width, m_height;

  // Extrinsics (Eigen)
  Eigen::Matrix3d m_basis;  // Rotation
  Eigen::Vector3d m_origin; // Translation (World Position)
};
