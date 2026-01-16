#include "Reconstructor.hpp"
#include <iostream>

#include <filesystem>
namespace fs = std::filesystem;
#include "Decoder.hpp"

int main(int argc, char *argv[]) {
  // We now expect 3 or 4 arguments: camera, projector, input, [output]
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <camera.json> <projector.json> <input_folder> [output.ply]"
              << std::endl;
    return 1;
  }

  std::string cam_file = argv[1];
  std::string proj_file = argv[2];
  std::string input_path = argv[3];
  std::string output_ply = (argc >= 5) ? argv[4] : "output.ply";

  try {
    Reconstructor recon(cam_file, proj_file);

    if (fs::is_directory(input_path)) {
      // Mode 2: Real Decoding
      std::cout << "Running Decoder..." << std::endl;

      Decoder decoder;
      // Configure cropping to remove background walls (matches Python defaults)
      decoder.setCrop(1600, -150);
      decoder.decodeSequence(input_path);

      std::cout << "Reconstructing..." << std::endl;
      recon.processMatches(decoder.getMatches());

    } else {
      std::cerr << "Error: Input must be a directory containing EXR images."
                << std::endl;
      return 1;
    }

    recon.saveToPLY(output_ply);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
