import cv2
import sys
import numpy as np

def inspect_exr(path):
    print(f"Inspecting with OpenCV: {path}")
    
    # Try imreadmulti first (for multi-page)
    retval, mats = cv2.imreadmulti(path, flags=cv2.IMREAD_UNCHANGED)
    if retval:
        print(f"imreadmulti found {len(mats)} pages/frames.")
        for i, m in enumerate(mats):
            print(f"Frame {i}: shape={m.shape}, dtype={m.dtype}")
    else:
        print("imreadmulti returned False. Trying standard imread...")
        # Try standard imread
        img = cv2.imread(path, cv2.IMREAD_UNCHANGED)
        if img is None:
            print("Failed to load image.")
            return
        
        print(f"Image shape: {img.shape}")
        print(f"Image dtype: {img.dtype}")
        
        # Check channels
        if len(img.shape) > 2:
            print(f"Channels: {img.shape[2]}")
        else:
            print("Single channel.")

if __name__ == "__main__":
    inspect_exr(sys.argv[1])
