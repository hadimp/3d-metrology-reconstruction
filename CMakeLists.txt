cmake_minimum_required(VERSION 3.14)
project(3d-metrology-reconstruction LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        INTERFACE_COMPILE_OPTIONS "-Xpreprocessor -fopenmp"
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

add_executable(metrology-recon src/main.cpp src/Camera.cpp src/Intersection.cpp src/Reconstructor.cpp src/Decoder.cpp)
target_link_libraries(metrology-recon PRIVATE Eigen3::Eigen nlohmann_json::nlohmann_json ${OpenCV_LIBS})

if (APPLE)
    target_compile_options(metrology-recon PRIVATE "-Xpreprocessor" "-fopenmp")
    target_link_libraries(metrology-recon PRIVATE "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    target_include_directories(metrology-recon PRIVATE "/opt/homebrew/opt/libomp/include")
else()
    target_link_libraries(metrology-recon PRIVATE OpenMP::OpenMP_CXX)
endif()


# Create a compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
