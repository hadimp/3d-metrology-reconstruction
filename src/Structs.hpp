#pragma once

/**
 * Represents a pair of corresponding 2D coordinates between
 * the camera and the projector.
 */
struct Match {
  double cam_u;  // Camera pixel X
  double cam_v;  // Camera pixel Y
  double proj_u; // Projector pixel X
  double proj_v; // Projector pixel Y
};
