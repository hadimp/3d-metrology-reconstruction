#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <Eigen/Dense>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_camera_geometry.json>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }

    try {
        json data = json::parse(f);
        
        // Check if "mtx" key exists
        if (data.contains("mtx")) {
            std::cout << "Successfully parsed calibration file!" << std::endl;
            
            // Just a simple print to prove we can read the structure
            auto mtx_raw = data["mtx"];
            std::cout << "Camera Matrix (Raw JSON):" << std::endl;
            std::cout << mtx_raw.dump(4) << std::endl;
            
            // Ideally we would map this to an Eigen matrix here, 
            // but for step 1 let's just prove we can read it.
            
            double focal_x = mtx_raw[0][0];
            double focal_y = mtx_raw[1][1];
            
            std::cout << "\nExtracted Intrinsics:\n";
            std::cout << "Focal Length X: " << focal_x << "\n";
            std::cout << "Focal Length Y: " << focal_y << std::endl;
        } else {
            std::cerr << "Error: JSON does not contain 'mtx' key." << std::endl;
        }

    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
