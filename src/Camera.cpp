#include "Camera.hpp"

Camera::Camera(const std::string &calibration_file) {
  std::ifstream f(calibration_file);
  if (!f.is_open()) {
    throw std::runtime_error("Could not open calibration file: " +
                             calibration_file);
  }

  json data = json::parse(f);

  // 1. Parse Intrinsics (Camera Matrix)
  // "mtx" is a 3x3 matrix in the JSON
  if (data.contains("mtx")) {
    m_cameraMatrix = cv::Mat(3, 3, CV_64F);
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        m_cameraMatrix.at<double>(r, c) = data["mtx"][r][c].get<double>();
      }
    }
  }

  // 2. Parse Distortion Coefficients
  // "dist" is usually a 1x5 or 1x12 array
  if (data.contains("dist")) {
    auto dist_json = data["dist"];
    m_distCoeffs = cv::Mat(1, dist_json.size(), CV_64F);
    for (size_t i = 0; i < dist_json.size(); ++i) {
      m_distCoeffs.at<double>(0, i) = dist_json[i].get<double>();
    }
  }

  // 3. Parse Extrinsics (Basis and Origin)
  // "basis" is 3x3 (Rotation from Camera to World)
  // "origin" is 3x1 (Translation)
  if (data.contains("extrinsic")) {
    // Some files might handle it differently, checking structure:
    // Assuming root keys 'basis' and 'origin' based on user's data description
  }

  // NOTE: Based on previous file reads, keys are at root: "basis", "origin"
  if (data.contains("basis")) {
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        m_basis(r, c) = data["basis"][r][c].get<double>();
      }
    }
  } else {
    m_basis.setIdentity(); // Default to identity if missing
  }

  if (data.contains("origin")) {
    for (int i = 0; i < 3; ++i) {
      m_origin(i) = data["origin"][i].get<double>();
    }
  } else {
    m_origin.setZero();
  }

  if (data.contains("width"))
    m_width = data["width"];
  if (data.contains("height"))
    m_height = data["height"];
}

Eigen::Vector3d Camera::pixelToRay(double u, double v) {
  // Step 1: Undistort point
  std::vector<cv::Point2f> distorted_points;
  distorted_points.push_back(cv::Point2f((float)u, (float)v));

  std::vector<cv::Point2f> undistorted_normalized;

  // cv::undistortPoints does the heavy lifting:
  // It applies the inverse of Intrinsic Matrix AND the inverse of Distortion
  // Model Output is in "Normalized Image Coordinates" (x/z, y/z) where z=1.
  cv::undistortPoints(distorted_points, undistorted_normalized, m_cameraMatrix,
                      m_distCoeffs);

  // Step 2: Convert to Ray in Camera Local Space
  // Normalized means z is implicitly 1.0
  Eigen::Vector3d ray_camera;
  ray_camera << undistorted_normalized[0].x, undistorted_normalized[0].y, 1.0;

  // Step 3: Transform to World Space
  // Ray_World = Rotation * Ray_Camera
  Eigen::Vector3d ray_world = m_basis * ray_camera;

  // Step 4: Normalize direction
  return ray_world.normalized();
}
