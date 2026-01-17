#include "Intersection.hpp"
#include <iostream>

Eigen::Vector3d intersectRays(const Eigen::Vector3d &o1,
                              const Eigen::Vector3d &d1,
                              const Eigen::Vector3d &o2,
                              const Eigen::Vector3d &d2) {
  // Vector connecting the two origins
  Eigen::Vector3d c = o2 - o1;

  double d1_dot_d2 = d1.dot(d2);
  double d1_dot_d1 = d1.dot(d1);
  double d2_dot_d2 = d2.dot(d2);

  // Determinant for solving the linear system (skew lines)
  double det = d1_dot_d1 * d2_dot_d2 - d1_dot_d2 * d1_dot_d2;

  double t1 = 0.0;
  double t2 = 0.0;

  if (std::abs(det) < 1e-6) {
    // Parallel lines: no unique intersection, return midpoint of origins
    return (o1 + o2) / 2.0;
  } else {
    double c_dot_d1 = c.dot(d1);
    double c_dot_d2 = c.dot(d2);

    // Compute parameters t1, t2 to find the closest points P1, P2 on each ray
    t1 = (c_dot_d1 * d2_dot_d2 - c_dot_d2 * d1_dot_d2) / det;
    t2 = (c_dot_d1 * d1_dot_d2 - c_dot_d2 * d1_dot_d1) / det;
  }

  Eigen::Vector3d P1 = o1 + t1 * d1;
  Eigen::Vector3d P2 = o2 + t2 * d2;

  // Final point is the midpoint of the shortest segment between rays
  return (P1 + P2) / 2.0;
}
