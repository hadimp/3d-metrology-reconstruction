# 3D Metrology Reconstruction Engine

A high-performance C++17 engine for 3D reconstruction using calibrated structured light patterns. This project focuses on high-precision metrology by triangulating correspondences between a camera and a projector.

<div align="center">
  
| 2D Pattern Projection (Input) | 3D Point Cloud (Output) |
|:---:|:---:|
| <img src="media/2d_input.png" height="300"> | <img src="media/3d_output.png" height="300"> |

</div>

## Key Features

- **Gray Code Decoding**: Robust decoding of binary-coded structured light patterns.
- **Camera/Projector Calibration**: Full support for intrinsic matrices and lens distortion coefficients.
- **Geometric Triangulation**: Fast computation of 3D point clouds from 2D pixel-to-pixel matches.
- **Sparse Point Clouds**: Efficient handling and export of high-density geometric data.

## Performance Comparison: C++ vs Python

The C++ engine was built to handle millions of points per scan with industrial-level throughput. Below is a benchmark comparing the optimized C++ engine against the original [Python (NumPy) implementation](https://github.com/geometryprocessing/scanner-sim) on an Apple M4.

| Metric | Python (NumPy) | C++ (Optimized) | Improvement |
| :--- | :--- | :--- | :--- |
| **Point Density** | ~454K points | ~9.28M points | 20.4x Density |
| **Decoding (Per Point)** | 28.61 µs/pt | 0.93 µs/pt | 30.7x Faster |
| **Triangulation (Per Point)**| 0.89 µs/pt | 0.18 µs/pt | 4.9x Faster |
| **Total Pipeline (Per Point)**| 29.50 µs/pt | 1.11 µs/pt | 26.5x Efficiency |

### How We Achieved This Boost

To achieve a **26x throughput improvement**, the engine uses several high-performance C++ techniques:

1.  **Multi-threaded Parallelism (OpenMP)**:
    - We parallelized the decoding loops and the triangulation step across all available CPU cores.
    - Used thread-local "bucket" storage for matches to prevent "mutex-locking" (contention) which usually kills multi-threaded performance.
2.  **Hardware-Level Vectorization (SIMD)**:
    - Allowed the compiler to use specific ARM NEON instructions.
    - Leveraged Eigen's optimized math routines, allowing the CPU to process 4-8 coordinates in a single clock cycle.
3.  **Memory Management**:
    - Eliminated performance-degrading reallocations, ensuring heap memory is allocated once before the massive processing loops begin.

## Building the Engine

The C++ engine requires **OpenCV**, **Eigen**, and a C++17 compiler.

```bash
mkdir build && cd build
cmake ..
make -j8
```

## Usage

After building the engine, use the provided Python scripts to automate the multi-view reconstruction and merge.

```bash
# 1. Run full 360° reconstruction
python3 scripts/run_all.py

# 2. Generate interactive 3D visualization
python3 scripts/visualize_interactive.py --input media/full_model.ply
```

## Data Source

The algorithms and test data are based on the [Scanner-Sim](https://geometryprocessing.github.io/scanner-sim/) project, which provides a comprehensive dataset and simulator for structured light 3D metrology.
