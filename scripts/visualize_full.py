import sys
import numpy as np
import plotly.graph_objects as go
import os

def visualize_full(ply_path, out_html, target_points=100000):
    print(f"Reading {ply_path}...")
    
    all_points = []
    
    with open(ply_path, 'r') as f:
        # Skip header
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
            
        # Count points to optimize subsampling
        # (Though we already know it's ~16.3M)
        # We'll just read everything and then subsample. 
        # For 16M points, numpy array will take ~384MB (16e6 * 3 * 8 bytes).
        # Python list of lists will take much more.
        
        # To be memory efficient, we can use Reservoir Sampling or just block-subsample.
        # Let's try reading and append to list first.
        for line in f:
            vals = line.strip().split()
            if len(vals) < 3: continue
            all_points.append([float(vals[0]), float(vals[1]), float(vals[2])])

    points = np.array(all_points)
    print(f"Loaded {len(points)} points.")
    
    if len(points) > target_points:
        idx = np.random.choice(len(points), target_points, replace=False)
        plot_points = points[idx]
        print(f"Subsampled to {target_points} points.")
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
            opacity=0.8
        )
    )])
    
    fig.update_layout(
        title=f"Full 360° Reconstruction ({len(points)} total points)",
        scene=dict(
            aspectmode='data',
            xaxis_title='X',
            yaxis_title='Y',
            zaxis_title='Z'
        ),
        margin=dict(l=0, r=0, b=0, t=40)
    )
    
    fig.write_html(out_html)
    print(f"Saved visualization to {out_html}")

if __name__ == "__main__":
    ply_input = "results/full_model.ply"
    html_output = "results/full_model_visualization.html"
    
    if not os.path.exists(ply_input):
        print(f"Error: {ply_input} not found.")
        sys.exit(1)
        
    visualize_full(ply_input, html_output)
