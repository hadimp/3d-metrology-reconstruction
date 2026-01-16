import sys
import numpy as np

def verify(filename):
    print(f"Scanning {filename}...")
    count = 0
    in_range_count = 0
    min_z = float('inf')
    max_z = float('-inf')
    
    in_range_min = np.array([float('inf')]*3)
    in_range_max = np.array([float('-inf')]*3)

    with open(filename, 'r') as f:
        # Skip header
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
            
        # Read points
        for line in f:
            vals = line.strip().split()
            if len(vals) < 3: continue
            
            x, y, z = float(vals[0]), float(vals[1]), float(vals[2])
            count += 1
            
            if z < min_z: min_z = z
            if z > max_z: max_z = z
            
            if 800 <= z <= 900:
                in_range_count += 1
                p = np.array([x, y, z])
                in_range_min = np.minimum(in_range_min, p)
                in_range_max = np.maximum(in_range_max, p)
                
            if count % 1000000 == 0:
                print(f"Processed {count} points...", end='\r')

    print(f"\nTotal Points: {count}")
    print(f"Global Z Range: {min_z:.2f} to {max_z:.2f}")
    print(f"Points in [800, 900]: {in_range_count}")
    if in_range_count > 0:
        print(f"In-Range Bounds:")
        print(f"  X: {in_range_min[0]:.4f} to {in_range_max[0]:.4f}")
        print(f"  Y: {in_range_min[1]:.4f} to {in_range_max[1]:.4f}")
        print(f"  Z: {in_range_min[2]:.4f} to {in_range_max[2]:.4f}")

if __name__ == "__main__":
    verify("output.ply")
