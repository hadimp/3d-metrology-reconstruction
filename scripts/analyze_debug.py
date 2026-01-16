import os
import cv2
import numpy as np

def analyze_maps():
    print("--- Analyzing Debug Maps ---")
    
    # Enable OpenEXR
    os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"
    
    # Load debug maps
    try:
        map_h = cv2.imread("build/debug_proj_row.exr", cv2.IMREAD_UNCHANGED)
        map_v = cv2.imread("build/debug_proj_col.exr", cv2.IMREAD_UNCHANGED)
    except Exception as e:
        print(f"CV2 Exception: {e}")
        map_h = None
    
    mask = cv2.imread("build/debug_mask.png", cv2.IMREAD_GRAYSCALE)
    
    if map_h is None or map_v is None:
        print("Error: Could not load debug maps (likely OpenCV EXR missing). Skipping Map Analysis.")
        return

    # Check mask coverage
    valid_pixels = cv2.countNonZero(mask)
    total_pixels = mask.shape[0] * mask.shape[1]
    print(f"Mask Coverage: {valid_pixels} / {total_pixels} ({valid_pixels/total_pixels*100:.2f}%)")
    
    # Stats on valid pixels
    vals_h = map_h[mask > 0]
    vals_v = map_v[mask > 0]
    
    print(f"H-Map (Rows) - Min: {np.min(vals_h)}, Max: {np.max(vals_h)}")
    print(f"V-Map (Cols) - Min: {np.min(vals_v)}, Max: {np.max(vals_v)}")
    
    # Check monotonicity / noise
    # Calculate gradient on a small valid patch
    print("Checking Gradients (Monotonicity)...")
    
    # Find a roi
    y, x = np.where(mask > 0)
    if len(y) > 0:
        roi = map_v[np.min(y):np.max(y), np.min(x):np.max(x)]
        grad_x = cv2.Sobel(roi, cv2.CV_64F, 1, 0, ksize=5)
        print(f"V-Map Grad X Mean: {np.mean(grad_x)}")
        
        roi_h = map_h[np.min(y):np.max(y), np.min(x):np.max(x)]
        grad_y = cv2.Sobel(roi_h, cv2.CV_64F, 0, 1, ksize=5)
        print(f"H-Map Grad Y Mean: {np.mean(grad_y)}")
        
def analyze_ply():
    print("\n--- Analyzing Point Cloud ---")
    pts = []
    with open("build/output.ply", 'r') as f:
        read = False
        for line in f:
            if "end_header" in line:
                read = True
                continue
            if read:
                parts = line.strip().split()
                if len(parts) >= 3:
                     pts.append([float(parts[0]), float(parts[1]), float(parts[2])])
                # Limit for speed
                if len(pts) > 100000: break
    
    if not pts:
        print("No points found!")
        return

    pts = np.array(pts)
    print(f"Loaded {len(pts)} points (subsample).")
    print("Bounding Box:")
    print(f"X: {np.min(pts[:,0]):.4f} to {np.max(pts[:,0]):.4f}")
    print(f"Y: {np.min(pts[:,1]):.4f} to {np.max(pts[:,1]):.4f}")
    print(f"Z: {np.min(pts[:,2]):.4f} to {np.max(pts[:,2]):.4f}")

if __name__ == "__main__":
    analyze_maps()
    analyze_ply()
