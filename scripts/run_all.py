import subprocess
import os
import numpy as np
import json
import argparse
from scipy.spatial.transform import Rotation as R

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

def main():
    parser = argparse.ArgumentParser(description="Process multi-view scans and merge into a full model.")
    parser.add_argument("--base_dir", type=str, default="../data/avocado_30_deg", help="Directory containing position folders")
    parser.add_argument("--results_dir", type=str, default="media", help="Directory to save partial and full results")
    parser.add_argument("--camera", type=str, default="../data/calibrations/camera_geometry.json", help="Camera calibration JSON")
    parser.add_argument("--projector", type=str, default="../data/calibrations/projector_geometry.json", help="Projector calibration JSON")
    parser.add_argument("--stage", type=str, default="../data/calibrations/stage_geometry.json", help="Stage geometry JSON")
    parser.add_argument("--z_min", type=float, default=810.0, help="Minimum Z for filtering")
    parser.add_argument("--z_max", type=float, default=890.0, help="Maximum Z for filtering")
    parser.add_argument("--recon_bin", type=str, default="./metrology-recon", help="Path to the metrology-recon executable")
    
    args = parser.parse_args()

    os.makedirs(args.results_dir, exist_ok=True)
    
    with open(args.stage, 'r') as f:
        stage_calib = json.load(f)
    
    stage_p = np.array(stage_calib["p"])
    stage_dir = np.array(stage_calib["dir"])
    
    all_points = []
    
    # Process all 12 positions (0, 30, 60 ...)
    for i in range(12):
        angle_deg = i * 30
        folder_name = f"position_{angle_deg}"
        input_folder = os.path.join(args.base_dir, folder_name, "gray")
        output_ply = os.path.join(args.results_dir, f"partial_{angle_deg}.ply")
        
        print(f"--- Processing View {angle_deg}° ({i}/11) ---")
        
        if not os.path.exists(output_ply):
            cmd = [args.recon_bin, args.camera, args.projector, input_folder, output_ply]
            subprocess.check_call(cmd)
        else:
            print(f"Skipping View {angle_deg}° (already exists)")
            
        # 2. Load Partial Cloud
        points = load_ply(output_ply)
        
        # 3. Filter Background (Z-range) and ensure finite points
        mask = (points[:, 2] >= args.z_min) & (points[:, 2] <= args.z_max)
        finite_mask = np.all(np.isfinite(points), axis=1)
        final_mask = mask & finite_mask
        filtered_points = points[final_mask]
        
        if len(filtered_points) < len(points):
            print(f"  Note: Filtered out {len(points) - len(filtered_points)} invalid/out-of-range points.")

        # 4. Rotate to Canonical Frame using Stage Geometry
        angle_rad = (angle_deg * np.pi / 180.0)
        rot = R.from_rotvec(-angle_rad * stage_dir)
        
        centered_points = filtered_points - stage_p
        rotated_points = rot.apply(centered_points) + stage_p
        
        all_points.append(rotated_points)
        print(f"View {angle_deg}°: {len(rotated_points)} points merged.")

    # 5. Save Full Model
    if all_points:
        full_cloud = np.vstack(all_points)
        full_path = os.path.join(args.results_dir, "full_model.ply")
        save_ply(full_cloud, full_path)
        
        print(f"--- Done! ---")
        print(f"Full model saved to {full_path}")
        print(f"Total points: {len(full_cloud)}")
    else:
        print("No points were generated.")

if __name__ == "__main__":
    main()
