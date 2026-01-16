import numpy as np
import os

def load_ply(filename):
    points = []
    with open(filename, 'r') as f:
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
        for line in f:
            vals = line.strip().split()
            if len(vals) >= 3:
                points.append([float(vals[0]), float(vals[1]), float(vals[2])])
    return np.array(points)

def rotation_matrix_y(angle_deg):
    rad = np.radians(angle_deg)
    c, s = np.cos(rad), np.sin(rad)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])

stage_p = np.array([-10.5178, -40.0557, 858.6701])

for i in range(12):
    angle = i*30
    path = f"results/partial_{angle}.ply"
    if not os.path.exists(path): continue
    
    points = load_ply(path)
    mask = (points[:, 2] >= 810) & (points[:, 2] <= 890)
    p = points[mask]
    
    # Current Alignment ( -angle )
    R = rotation_matrix_y(-angle)
    p_ref = ((p - stage_p) @ R.T) + stage_p
    
    # Alternate Alignment ( +angle )
    R_alt = rotation_matrix_y(+angle)
    p_alt = ((p - stage_p) @ R_alt.T) + stage_p
    
    bbox_ref = [np.min(p_ref, axis=0), np.max(p_ref, axis=0)]
    bbox_alt = [np.min(p_alt, axis=0), np.max(p_alt, axis=0)]
    
    print(f"Angle {angle}:")
    print(f"  Ref BBox: {bbox_ref[0]} to {bbox_ref[1]}")
    print(f"  Alt BBox: {bbox_alt[0]} to {bbox_alt[1]}")
