#include "Camera.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_calibration.json>"
              << std::endl;
    return 1;
  }

  std::string filename = argv[1];

  try {
    std::cout << "Loading Camera from: " << filename << std::endl;
    Camera cam(filename);

    std::cout << "Successfully loaded camera parameters." << std::endl;
    std::cout << "Camera Origin: " << cam.getPosition().transpose()
              << std::endl;

    // Test: Project center of image
    // Assuming ~4000x3000 image, let's pick center
    double u = 2000.0;
    double v = 1500.0;

    Eigen::Vector3d ray = cam.pixelToRay(u, v);

    std::cout << "---------------------------------" << std::endl;
    std::cout << "Test Projection at (" << u << ", " << v << "):" << std::endl;
    std::cout << "Computed Ray Direction: " << ray.transpose() << std::endl;
    std::cout << "---------------------------------" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
