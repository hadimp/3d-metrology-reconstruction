#include "Decoder.hpp"
#include <iostream>
#include <omp.h>

Decoder::Decoder() {}

/**
 * Converts a Gray-coded integer to its binary equivalent.
 * Uses a standard bitwise XOR shift trick.
 */
unsigned int Decoder::grayToBinary(unsigned int num) {
  num = num ^ (num >> 16);
  num = num ^ (num >> 8);
  num = num ^ (num >> 4);
  num = num ^ (num >> 2);
  num = num ^ (num >> 1);
  return num;
}

void Decoder::decodeSequence(const std::string &folder_path) {
  m_matches.clear();

  std::string path_white = folder_path + "/img_00.exr";
  std::string path_blank = folder_path + "/img_01.exr";

  cv::Mat img_white = cv::imread(path_white, cv::IMREAD_UNCHANGED);
  cv::Mat img_blank = cv::imread(path_blank, cv::IMREAD_UNCHANGED);

  if (img_white.empty() || img_blank.empty()) {
    std::cerr << "Error: Could not load white/blank reference images."
              << std::endl;
    return;
  }

  if (img_white.channels() > 1)
    cv::cvtColor(img_white, img_white, cv::COLOR_BGR2GRAY);
  if (img_blank.channels() > 1)
    cv::cvtColor(img_blank, img_blank, cv::COLOR_BGR2GRAY);

  // Create a mask of pixels that have enough signal contrast
  cv::Mat img_diff = img_white - img_blank;
  double signal_threshold = 0.05;
  cv::threshold(img_diff, m_mask, signal_threshold, 1.0, cv::THRESH_BINARY);
  m_mask.convertTo(m_mask, CV_8U);

  // Noise reduction via morphological erosion
  cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
  cv::morphologyEx(m_mask, m_mask, cv::MORPH_ERODE, element, cv::Point(-1, -1),
                   6);

  if (m_crop > 0) {
    m_mask.colRange(0, m_crop) = 0;

    int cols = m_mask.cols;
    int right_limit = cols - m_crop + 2 * m_offset;
    if (right_limit < cols) {
      m_mask.colRange(right_limit, cols) = 0;
    }
  }

  int rows = img_white.rows;
  int cols = img_white.cols;

  cv::Mat code_V = cv::Mat::zeros(rows, cols, CV_32S);
  cv::Mat code_H = cv::Mat::zeros(rows, cols, CV_32S);

  /**
   * Helper to decode a sequence of binary pattern images into a bitmask.
   * Compares each pattern with its inverse to decide if a bit is 1 or 0.
   */
  auto decode_bits = [&](int start_idx, int end_idx, cv::Mat &code_map) {
    int bit = 0;

    // Pairs of (pattern, inverse_pattern) are stored sequentially
    for (int i = end_idx - 2; i >= start_idx; i -= 2) {
      char buf1[32], buf2[32];
      snprintf(buf1, sizeof(buf1), "/img_%02d.exr", i);
      snprintf(buf2, sizeof(buf2), "/img_%02d.exr", i + 1);

      cv::Mat pattern = cv::imread(folder_path + buf1, cv::IMREAD_UNCHANGED);
      cv::Mat inv_pattern =
          cv::imread(folder_path + buf2, cv::IMREAD_UNCHANGED);

      if (pattern.empty() || inv_pattern.empty()) {
        std::cerr << "Missing pattern pair: " << i << std::endl;
        continue;
      }

      if (pattern.channels() > 1)
        cv::cvtColor(pattern, pattern, cv::COLOR_BGR2GRAY);
      if (inv_pattern.channels() > 1)
        cv::cvtColor(inv_pattern, inv_pattern, cv::COLOR_BGR2GRAY);

#pragma omp parallel for
      for (int r = 0; r < rows; ++r) {
        float *p_row = pattern.ptr<float>(r);
        float *inv_row = inv_pattern.ptr<float>(r);
        int *code_row = code_map.ptr<int>(r);
        uint8_t *mask_row = m_mask.ptr<uint8_t>(r);

        for (int c = 0; c < cols; ++c) {
          if (mask_row[c]) {
            if (p_row[c] > inv_row[c]) {
              code_row[c] |= (1 << bit);
            }
          }
        }
      }
      bit++;
    }
  };

  // Vertical images
  std::cout << "  Decoding Vertical Bits..." << std::endl;
  decode_bits(2, 24, code_V);

  // Horizontal images
  std::cout << "  Decoding Horizontal Bits..." << std::endl;
  decode_bits(24, 46, code_H);

  // Gray to Binary & Generate Matches
  std::cout << "  Finalizing matches..." << std::endl;

  // Use thread-local storage to avoid push_back contention
  int max_threads = omp_get_max_threads();
  std::vector<std::vector<Match>> thread_local_matches(max_threads);

#pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    std::vector<Match> &local_matches = thread_local_matches[thread_id];

#pragma omp for
    for (int r = 0; r < rows; ++r) {
      int *row_v = code_V.ptr<int>(r);
      int *row_h = code_H.ptr<int>(r);
      uint8_t *mask_row = m_mask.ptr<uint8_t>(r);

      for (int c = 0; c < cols; ++c) {
        if (mask_row[c]) {
          unsigned int gray_v = (unsigned int)row_v[c];
          unsigned int gray_h = (unsigned int)row_h[c];

          if (gray_v > 0 && gray_h > 0) {
            unsigned int bin_v = grayToBinary(gray_v);
            unsigned int bin_h = grayToBinary(gray_h);

            Match m;
            m.cam_u = (double)c;
            m.cam_v = (double)r;
            m.proj_u = (double)bin_v - 64.0;
            m.proj_v = (double)bin_h - 484.0;

            local_matches.push_back(m);
          }
        }
      }
    }
  }

  // Merge the buckets
  size_t total_matches = 0;
  for (const auto &bucket : thread_local_matches) {
    total_matches += bucket.size();
  }
  m_matches.reserve(total_matches);
  for (auto &bucket : thread_local_matches) {
    m_matches.insert(m_matches.end(), bucket.begin(), bucket.end());
  }
}
