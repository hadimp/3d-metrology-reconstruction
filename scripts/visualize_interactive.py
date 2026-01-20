import sys
import numpy as np
import plotly.graph_objects as go
import os
import argparse

def visualize_full(ply_path, out_html, target_points=100000):
    print(f"Reading {ply_path}...")
    
    with open(ply_path, 'rb') as f:
        # Read header to find vertex count
        header = ""
        while "end_header" not in header:
            line = f.readline().decode('ascii', errors='ignore')
            header += line
            if "element vertex" in line:
                num_verts = int(line.split()[-1])
        
        # Read the rest as binary data
        data = f.read()
        points = np.frombuffer(data, dtype=np.float32).reshape(-1, 3)
    
    print(f"Loaded {len(points)} points.")
    
    if len(points) > target_points:
        idx = np.random.choice(len(points), target_points, replace=False)
        plot_points = points[idx]
        print(f"Subsampled to {target_points} points for visualization.")
    else:
        plot_points = points

    print("Generating Plotly visualization...")
    fig = go.Figure(data=[go.Scatter3d(
        x=plot_points[:,0],
        y=plot_points[:,1],
        z=plot_points[:,2],
        mode='markers',
        marker=dict(
            size=1,
            color=plot_points[:,2],
            colorscale='Viridis',
            opacity=0.8,
            colorbar=dict(title="Z (mm)")
        )
    )])
    
    fig.update_layout(
        title=f"360° Reconstruction - Avocado Model ({len(points)} total points)",
        scene=dict(
            aspectmode='data',
            xaxis_title='X (mm)',
            yaxis_title='Y (mm)',
            zaxis_title='Z (mm)'
        ),
        margin=dict(l=0, r=0, b=0, t=40)
    )
    
    fig.write_html(out_html)
    print(f"Saved interactive visualization to {out_html}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate interactive 3D visualization from PLY point cloud.")
    parser.add_argument("--input", type=str, default="results/full_model.ply", help="Path to input PLY file")
    parser.add_argument("--output", type=str, default="results/360_reconstruction_interactive.html", help="Path to output HTML file")
    parser.add_argument("--points", type=int, default=100000, help="Maximum points to visualize (for performance)")
    
    args = parser.parse_args()
    
    visualize_full(args.input, args.output, args.points)
