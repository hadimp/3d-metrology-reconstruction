#pragma once

#include "Structs.hpp"
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class Decoder {
public:
  Decoder();

  /**
   * Decodes a structured light sequence from a folder.
   * Expects standard naming: img_00.exr ... img_45.exr
   */
  void decodeSequence(const std::string &folder_path);

  /**
   * Returns the computed matches (Camera -> Projector).
   */
  const std::vector<Match> &getMatches() const { return m_matches; }

  // Set cropping parameters to ignore background (matches Python logic)
  void setCrop(int crop, int offset) {
    m_crop = crop;
    m_offset = offset;
  }

  /**
   * Debug: Save the decoded maps and masks for inspection.
   */
  void saveDebugMaps(const std::string &output_folder);

private:
  std::vector<Match> m_matches;
  cv::Mat m_mask;     // Valid pixels
  cv::Mat m_visual_H; // For debugging
  cv::Mat m_visual_V; // For debugging

  // Cropping parameters
  int m_crop = 0;
  int m_offset = 0;

  // Helper: Gray Code to Binary conversion
  unsigned int grayToBinary(unsigned int num);
};
