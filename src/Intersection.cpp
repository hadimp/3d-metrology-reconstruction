#include "Intersection.hpp"
#include <iostream>

Eigen::Vector3d intersectRays(const Eigen::Vector3d &o1,
                              const Eigen::Vector3d &d1,
                              const Eigen::Vector3d &o2,
                              const Eigen::Vector3d &d2) {
  // We want to solve for scalars t1 and t2 such that:
  // P1 = o1 + t1 * d1
  // P2 = o2 + t2 * d2
  // Minimize distance ||P1 - P2||

  // Vector connecting origins
  Eigen::Vector3d c = o2 - o1;

  // Dot products
  double d1_dot_d2 = d1.dot(d2);
  // Since directions are normalized, d1.dot(d1) == 1.0 and d2.dot(d2) == 1.0
  // But we'll use variables for clarity/robustness.
  double d1_dot_d1 = d1.dot(d1);
  double d2_dot_d2 = d2.dot(d2);

  // Denominator for Cramer's rule
  // det = 1 - (d1.d2)^2. If rays are parallel, det = 0.
  double det = d1_dot_d1 * d2_dot_d2 - d1_dot_d2 * d1_dot_d2;

  double t1 = 0.0;
  double t2 = 0.0;

  if (std::abs(det) < 1e-6) {
    // Rays are parallel (or very close to it)
    // We can't find a unique intersection. Return the midpoint of origins or
    // o1. For structured light, this essentially means "Infinite depth" or
    // invalid.
    return (o1 + o2) / 2.0;
  } else {
    // General Case: Skew Lines
    // t1 = ( (o2-o1).d1 * d2.d2 - (o2-o1).d2 * d1.d2 ) / det
    double c_dot_d1 = c.dot(d1);
    double c_dot_d2 = c.dot(d2);

    t1 = (c_dot_d1 * d2_dot_d2 - c_dot_d2 * d1_dot_d2) / det;
    t2 = (c_dot_d1 * d1_dot_d2 - c_dot_d2 * d1_dot_d1) / det;
  }

  // Points on the rays
  Eigen::Vector3d P1 = o1 + t1 * d1;
  Eigen::Vector3d P2 = o2 + t2 * d2;

  // Return the midpoint
  return (P1 + P2) / 2.0;
}
