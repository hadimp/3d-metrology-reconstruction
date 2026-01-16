import subprocess
import os
import numpy as np
import math

def save_ply(points, filename):
    with open(filename, 'w') as f:
        f.write("ply\n")
        f.write("format ascii 1.0\n")
        f.write(f"element vertex {len(points)}\n")
        f.write("property float x\n")
        f.write("property float y\n")
        f.write("property float z\n")
        f.write("end_header\n")
        for p in points:
            f.write(f"{p[0]} {p[1]} {p[2]}\n")

def load_ply(filename):
    points = []
    with open(filename, 'r') as f:
        # Skip header
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
        
        for line in f:
            vals = line.strip().split()
            if len(vals) >= 3:
                points.append([float(vals[0]), float(vals[1]), float(vals[2])])
    return np.array(points)

def rotation_matrix_y(angle_deg):
    rad = math.radians(angle_deg)
    c = math.cos(rad)
    s = math.sin(rad)
    # Rotation around Y-axis
    # | cos  0  sin |
    # |  0   1   0  |
    # |-sin  0  cos |
    return np.array([
        [c, 0, s],
        [0, 1, 0],
        [-s, 0, c]
    ])

import json
from scipy.spatial.transform import Rotation as R

def main():
    base_dir = "../data/avocado_30_deg"
    results_dir = "results"
    os.makedirs(results_dir, exist_ok=True)
    
    cam_json = "../data/calibrations/camera_geometry.json"
    proj_json = "../data/calibrations/projector_geometry.json"
    stage_json = "../data/calibrations/stage_geometry.json"
    
    with open(stage_json, 'r') as f:
        stage_calib = json.load(f)
    
    stage_p = np.array(stage_calib["p"])
    stage_dir = np.array(stage_calib["dir"])
    
    all_points = []
    
    # Process all 12 positions (0, 30, 60 ...)
    for i in range(12):
        angle_deg = i * 30
        folder_name = f"position_{angle_deg}"
        input_folder = os.path.join(base_dir, folder_name, "gray")
        output_ply = os.path.join(results_dir, f"partial_{angle_deg}.ply")
        
        print(f"--- Processing View {angle_deg}° ({i}/11) ---")
        
        if not os.path.exists(output_ply):
            cmd = ["./simple_recon", cam_json, proj_json, input_folder, output_ply]
            subprocess.check_call(cmd)
        else:
            print(f"Skipping View {angle_deg}° (already exists)")
            
        # 2. Load Partial Cloud
        points = load_ply(output_ply)
        
        # 3. Filter Background (Z-range [810, 890])
        mask = (points[:, 2] >= 810) & (points[:, 2] <= 890)
        filtered_points = points[mask]
        
        # 4. Rotate to Canonical Frame using Stage Geometry
        # Logic from original merge.py:
        # rot = R.from_rotvec((-angle_in_rad) * dir_vector)
        # centered_points = points - p0
        # p_rot = rot.apply(centered_points)
        
        angle_rad = (angle_deg * np.pi / 180.0)
        rot = R.from_rotvec(-angle_rad * stage_dir)
        
        centered_points = filtered_points - stage_p
        rotated_points = rot.apply(centered_points) + stage_p
        
        all_points.append(rotated_points)
        print(f"View {angle_deg}°: {len(rotated_points)} points merged.")

    # 5. Save Full Model
    full_cloud = np.vstack(all_points)
    full_path = os.path.join(results_dir, "full_model.ply")
    save_ply(full_cloud, full_path)
    
    print(f"--- Done! ---")
    print(f"Full model saved to {full_path}")
    print(f"Total points: {len(full_cloud)}")

if __name__ == "__main__":
    main()
