#pragma once

/**
 * Represents a pair of corresponding 2D coordinates between
 * the camera and the projector.
 */
struct Match {
  float cam_u;  // Camera pixel X
  float cam_v;  // Camera pixel Y
  float proj_u; // Projector pixel X
  float proj_v; // Projector pixel Y
};
