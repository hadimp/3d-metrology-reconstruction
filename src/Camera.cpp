#include "Camera.hpp"
#include <fstream>
#include <stdexcept>

Camera::Camera(const std::string &calibration_file) {
  std::ifstream f(calibration_file);
  if (!f.is_open()) {
    throw std::runtime_error("Could not open calibration file: " +
                             calibration_file);
  }

  json data = json::parse(f);

  // "mtx" is a 3x3 matrix in the JSON
  if (data.contains("mtx")) {
    m_cameraMatrix =
        cv::Mat(3, 3, CV_64F); // 3x3 Intrinsic matrix [fx 0 cx; 0 fy cy; 0 0 1]
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        m_cameraMatrix.at<double>(r, c) = data["mtx"][r][c].get<double>();
      }
    }
  }

  // "dist" is a 1x5 vector
  if (data.contains("dist")) {
    auto dist_json = data["dist"];
    m_distCoeffs =
        cv::Mat(1, dist_json.size(),
                CV_64F); // Lens distortion coefficients (k1, k2, p1, p2, k3)
    for (size_t i = 0; i < dist_json.size(); ++i) {
      m_distCoeffs.at<double>(0, i) = dist_json[i].get<double>();
    }
  }

  if (data.contains("basis")) {
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        m_basis(r, c) =
            data["basis"][r][c]
                .get<double>(); // Rotation basis for world coordinates
      }
    }
  } else {
    m_basis.setIdentity();
  }

  if (data.contains("origin")) {
    for (int i = 0; i < 3; ++i) {
      m_origin(i) =
          data["origin"][i].get<double>(); // Camera center in world coordinates
    }
  } else {
    m_origin.setZero();
  }

  if (data.contains("width"))
    m_width = data["width"]; // Image width
  if (data.contains("height"))
    m_height = data["height"]; // Image height
}

Eigen::Vector3d Camera::pixelToRay(double u, double v) {
  // 1. Convert pixel (u, v) to undistorted, normalized camera coordinates (x,
  // y, 1)
  std::vector<cv::Point2f> distorted_points;
  distorted_points.push_back(cv::Point2f((float)u, (float)v));

  std::vector<cv::Point2f> undistorted_normalized;
  cv::undistortPoints(distorted_points, undistorted_normalized, m_cameraMatrix,
                      m_distCoeffs);

  Eigen::Vector3d ray_camera;
  ray_camera << undistorted_normalized[0].x, undistorted_normalized[0].y, 1.0;

  // 2. Transform from camera coordinate system to world coordinate system
  // ray_world = R^T * ray_camera (where R is the camera-to-world rotation)
  Eigen::Vector3d ray_world = m_basis.transpose() * ray_camera;

  return ray_world.normalized();
}

Eigen::Vector3d Camera::getPosition() const { return m_origin; }
