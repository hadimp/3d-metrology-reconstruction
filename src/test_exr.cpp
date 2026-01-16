#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <image_path>" << std::endl;
    return 1;
  }

  std::string path = argv[1];
  std::cout << "Inspecting: " << path << std::endl;

  // 1. Try IMREAD_UNCHANGED to get standard channels
  cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
  if (!img.empty()) {
    std::cout << "--- cv::imread result ---" << std::endl;
    std::cout << "Dimensions: " << img.cols << "x" << img.rows << std::endl;
    std::cout << "Channels: " << img.channels() << std::endl;
    std::cout << "Depth: " << img.depth() << " (CV_32F=" << CV_32F << ")"
              << std::endl;
  } else {
    std::cerr << "cv::imread failed to load the image." << std::endl;
  }

  // 2. Try IMREADMULTI (Multi-page / Multi-channel tricky files)
  std::vector<cv::Mat> mats;
  bool success = cv::imreadmulti(path, mats, cv::IMREAD_UNCHANGED);
  if (success && !mats.empty()) {
    std::cout << "--- cv::imreadmulti result ---" << std::endl;
    std::cout << "Found " << mats.size() << " frames/pages." << std::endl;
    if (mats.size() > 0) {
      std::cout << "Frame 0 Dims: " << mats[0].cols << "x" << mats[0].rows
                << std::endl;
      std::cout << "Frame 0 Channels: " << mats[0].channels() << std::endl;
    }
  } else {
    std::cout << "cv::imreadmulti returned no multiple pages (or failed)."
              << std::endl;
  }

  return 0;
}
