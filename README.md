# 3D Metrology Reconstruction Engine

A high-performance C++17 engine for 3D reconstruction using calibrated structured light patterns. This project focuses on high-precision metrology by triangulating correspondences between a camera and a projector.

| 2D Pattern Projection (Input) | 3D Point Cloud (Output) |
|:---:|:---:|
| <img src="results/2d_input.png" height="300"> | <img src="results/3d_output.png" height="300"> |

## Key Features

- **Gray Code Decoding**: Robust decoding of binary-coded structured light patterns.
- **Camera/Projector Calibration**: Full support for intrinsic matrices and lens distortion coefficients.
- **Geometric Triangulation**: Fast computation of 3D point clouds from 2D pixel-to-pixel matches.
- **Sparse Point Clouds**: Efficient handling and export of high-density geometric data.

## Requirements

- **C++17** compatible compiler
- **OpenCV** (Core modules)
- **Eigen3** (Linear algebra)
- **nlohmann/json** (Calibration parsing)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./simple_recon <camera.json> <projector.json> <input_folder> [output.ply]
```

- `<camera.json>`: Standard camera intrinsics and distortion.
- `<projector.json>`: Projector calibration data (treated as a camera).
- `<input_folder>`: Directory containing the EXR pattern sequence.
- `[output.ply]`: (Optional) Path to save the resulting point cloud.

## Data Source

The algorithms and test data are based on the [Scanner-Sim](https://geometryprocessing.github.io/scanner-sim/) project, which provides a comprehensive dataset and simulator for structured light 3D metrology.
