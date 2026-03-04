cmake_minimum_required(VERSION 3.14)
project(3d-metrology-reconstruction LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    enable_language(OBJCXX)
endif()

# Find Eigen3
find_package(Eigen3 REQUIRED NO_MODULE)

# Find OpenCV
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/opencv")
find_package(OpenCV REQUIRED)

# Find OpenMP
if (APPLE)
    # macOS often needs help finding Homebrew's libomp
    set(OpenMP_DIR "/opt/homebrew/opt/libomp")
    find_library(OMP_LIB NAME omp PATHS "${OpenMP_DIR}/lib" REQUIRED)
    find_path(OMP_INCLUDE_DIR NAME omp.h PATHS "${OpenMP_DIR}/include" REQUIRED)
    
    add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
    set_target_properties(OpenMP::OpenMP_CXX PROPERTIES
        INTERFACE_COMPILE_OPTIONS "-Xpreprocessor;-fopenmp"
        INTERFACE_INCLUDE_DIRECTORIES "${OMP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${OMP_LIB}"
    )
else()
    find_package(OpenMP REQUIRED)
endif()

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Custom command to compile Metal shaders
if(APPLE)
    # Find Apple SDKs
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(METAL_LIBRARY_FW Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    set(APPLE_FRAMEWORKS ${FOUNDATION_LIBRARY} ${METAL_LIBRARY_FW} ${QUARTZCORE_LIBRARY} ${COREGRAPHICS_LIBRARY})
endif()

# We include the downloaded metal-cpp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/metal-cpp/metal-cpp)

add_executable(metrology-recon 
    src/main.cpp 
    src/Camera.cpp 
    src/Intersection.cpp 
    src/Reconstructor.cpp 
    src/Decoder.cpp 
    src/MetalContext.cpp)

if(APPLE)
    target_link_libraries(metrology-recon PRIVATE 
        Eigen3::Eigen 
        nlohmann_json::nlohmann_json 
        ${OpenCV_LIBS} 
        OpenMP::OpenMP_CXX 
        ${APPLE_FRAMEWORKS})
else()
    target_link_libraries(metrology-recon PRIVATE Eigen3::Eigen nlohmann_json::nlohmann_json ${OpenCV_LIBS} OpenMP::OpenMP_CXX)
endif()

# Optimization flags for speed and vectorization
if(NOT MSVC)
    target_compile_options(metrology-recon PRIVATE -O3 -march=native)
endif()

# Create a compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
