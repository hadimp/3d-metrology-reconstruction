#include "Decoder.hpp"
#include <iostream>

Decoder::Decoder() {}

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

  // 1. Load Reference Images
  // img_00: White (Pattern All On)
  // img_01: Blank (Pattern All Off)
  std::string path_white = folder_path + "/img_00.exr";
  std::string path_blank = folder_path + "/img_01.exr";

  cv::Mat img_white = cv::imread(path_white, cv::IMREAD_UNCHANGED);
  cv::Mat img_blank = cv::imread(path_blank, cv::IMREAD_UNCHANGED);

  if (img_white.empty() || img_blank.empty()) {
    std::cerr << "Error: Could not load white/blank reference images."
              << std::endl;
    return;
  }

  // Convert to float if not already
  // Note: OpenEXR loads as CV_32F usually.
  // If multiple channels, assume grayscale or pick Green channel?
  // Python script does 'make_gray=True'. Let's average channels or just pick
  // one.
  if (img_white.channels() > 1)
    cv::cvtColor(img_white, img_white, cv::COLOR_BGR2GRAY);
  if (img_blank.channels() > 1)
    cv::cvtColor(img_blank, img_blank, cv::COLOR_BGR2GRAY);

  // Compute basic Shadow Mask: (White - Blank) > Threshold
  cv::Mat diff = img_white - img_blank;

  // Simple global threshold (Unfiltered)
  double signal_threshold = 0.05;
  cv::threshold(diff, m_mask, signal_threshold, 1.0, cv::THRESH_BINARY);
  m_mask.convertTo(m_mask, CV_8U); // Convert to 8-bit boolean mask

  // 4. Morphological Erosion
  // Python: generate_binary_structure(2, 1) -> equivalent to 3x3 cross kernel
  // cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3))
  cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
  cv::morphologyEx(m_mask, m_mask, cv::MORPH_ERODE, element, cv::Point(-1, -1),
                   6); // 6 iterations

  // Apply Cropping (if configured)
  if (m_crop > 0) {
    // Python: clean[:, :crop] = 0
    m_mask.colRange(0, m_crop) = 0;

    // Python: clean[:, clean.shape[1] - crop + 2*offset:] = 0
    int cols = m_mask.cols; // Define cols here for cropping logic
    int right_limit = cols - m_crop + 2 * m_offset;
    if (right_limit < cols) {
      m_mask.colRange(right_limit, cols) = 0;
    }
  }

  // 2. Decode Patterns
  int rows = img_white.rows;
  int cols = img_white.cols;

  cv::Mat code_V =
      cv::Mat::zeros(rows, cols, CV_32S); // Vertical Code (Integer)
  cv::Mat code_H =
      cv::Mat::zeros(rows, cols, CV_32S); // Horizontal Code (Integer)

  auto decode_bits = [&](int start_idx, int end_idx, cv::Mat &code_map) {
    // Python: reversed(all_names[start:end:2])
    // Example Vertical: 2, 4, ... 22. Reversed -> 22, 20... 2.
    // Bit 0 corresponds to 22. Bit 10 corresponds to 2.

    int bit = 0;
    // Loop backwards from end_idx-2 down to start_idx, step 2
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

      // Bit is 1 if Pattern > Inverse
      // We can check this pixel-wise
      // code_map |= (1 << bit) where pattern > inv

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

  // Vertical images: 02..23 (Indices 2 to 24 in Python slice)
  decode_bits(2, 24, code_V);

  // Horizontal images: 24..45 (Indices 24 to 46 in Python slice)
  decode_bits(24, 46, code_H);

  // 3. Gray to Binary & Generate Matches
  int valid_matches = 0;

  // Debug output matrices

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

          // Python logic says:
          // p_r = bin_h (Projector Row comes from Horizontal Pattern?)
          // p_c = bin_v (Projector Col comes from Vertical Pattern?)
          // Let's verify standard:
          // Vertical Pattern = stripes are vertical -> Changes along X ->
          // Encodes Column. Horizontal Pattern = stripes are horizontal ->
          // Changes along Y -> Encodes Row. Yes. bin_v determines Projector
          // Column. bin_h determines Projector Row.

          // Python logic says:
          // if symmetric: p_r -= 1024 - 1080//2
          //               p_c -= 1024 - 1920//2
          // 11 bits = 2048 magnitude.
          // u_offset = 1024 - 960 = 64
          // v_offset = 1024 - 540 = 484

          Match m;
          m.cam_u = (double)c;
          m.cam_v = (double)r;
          m.proj_u = (double)bin_v - 64.0;
          m.proj_v = (double)bin_h - 484.0;

          m_matches.push_back(m);
          valid_matches++;
        }
      }
    }
  }
}
