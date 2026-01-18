cmake_minimum_required(VERSION 3.14)
project(3d-metrology-reconstruction LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Eigen3
find_package(Eigen3 REQUIRED NO_MODULE)

# Find OpenCV
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/opencv")
find_package(OpenCV REQUIRED)

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

add_executable(metrology-recon src/main.cpp src/Camera.cpp src/Intersection.cpp src/Reconstructor.cpp src/Decoder.cpp)
target_link_libraries(metrology-recon PRIVATE Eigen3::Eigen nlohmann_json::nlohmann_json ${OpenCV_LIBS})


# Create a compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
