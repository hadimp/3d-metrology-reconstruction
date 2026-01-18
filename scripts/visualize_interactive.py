import sys
import numpy as np
import plotly.graph_objects as go
import os
import argparse

def visualize_full(ply_path, out_html, target_points=100000):
    print(f"Reading {ply_path}...")
    
    all_points = []
    
    if not os.path.exists(ply_path):
        print(f"Error: {ply_path} not found.")
        sys.exit(1)

    with open(ply_path, 'r') as f:
        # Skip header
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
            
        for line in f:
            vals = line.strip().split()
            if len(vals) < 3: continue
            all_points.append([float(vals[0]), float(vals[1]), float(vals[2])])

    points = np.array(all_points)
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
