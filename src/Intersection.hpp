#pragma once

#include <Eigen/Dense>

/**
 * Computes the closest point (midpoint of the shortest segment) between two 3D
 * rays.
 */
Eigen::Vector3d intersectRays(const Eigen::Vector3d &o1,
                              const Eigen::Vector3d &d1,
                              const Eigen::Vector3d &o2,
                              const Eigen::Vector3d &d2);
