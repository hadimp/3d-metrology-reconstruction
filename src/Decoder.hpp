#pragma once

#include "Structs.hpp"
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * Handles decoding of structured light patterns (Gray code).
 * Extracts correspondences between camera pixels and projector pixels.
 */
class Decoder {
public:
  Decoder();

  void decodeSequence(const std::string &folder_path);

  const std::vector<Match> &getMatches() const { return m_matches; }

  void setCrop(int crop, int offset) {
    m_crop = crop;
    m_offset = offset;
  }

private:
  std::vector<Match> m_matches;
  cv::Mat m_mask;

  int m_crop = 0;
  int m_offset = 0;

  unsigned int grayToBinary(unsigned int num);
};
