#pragma once

#include <Eigen/Dense>

/**
 * Calculates the intersection (closest point) between two 3D rays.
 *
 * @param o1 Origin of Ray 1
 * @param d1 Direction of Ray 1 (Must be normalized)
 * @param o2 Origin of Ray 2
 * @param d2 Direction of Ray 2 (Must be normalized)
 * @return Eigen::Vector3d The midpoint of the shortest segment connecting the
 * two rays.
 */
Eigen::Vector3d intersectRays(const Eigen::Vector3d &o1,
                              const Eigen::Vector3d &d1,
                              const Eigen::Vector3d &o2,
                              const Eigen::Vector3d &d2);
