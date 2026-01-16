import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import plotly.graph_objects as go

def visualize_filtered(filename, out_html, out_png):
    print(f"Streaming {filename} and filtering [800, 900]...")
    points = []
    
    with open(filename, 'r') as f:
        # Skip header
        line = f.readline()
        while line and "end_header" not in line:
            line = f.readline()
            
        for line in f:
            vals = line.strip().split()
            if len(vals) < 3: continue
            
            z = float(vals[2])
            if 800 <= z <= 900:
                points.append([float(vals[0]), float(vals[1]), z])

    points = np.array(points)
    print(f"Found {len(points)} points in range.")
    
    # Subsample for visualization
    target = 30000
    if len(points) > target:
        idx = np.random.choice(len(points), target, replace=False)
        plot_points = points[idx]
    else:
        plot_points = points

    # Save PNG
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(plot_points[:,0], plot_points[:,1], plot_points[:,2], c=plot_points[:,2], cmap='viridis', s=1)
    ax.set_title(f"Corrected Reconstruction ({len(points)} pts in range)")
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.view_init(elev=-90, azim=-90)
    plt.savefig(out_png)
    print(f"Saved {out_png}")

    # Save HTML
    fig = go.Figure(data=[go.Scatter3d(
        x=plot_points[:,0],
        y=plot_points[:,1],
        z=plot_points[:,2],
        mode='markers',
        marker=dict(
            size=2,
            color=plot_points[:,2],
            colorscale='Viridis',
            opacity=0.8
        )
    )])
    fig.update_layout(title="Corrected Reconstruction", scene=dict(aspectmode='data'))
    fig.write_html(out_html)
    print(f"Saved {out_html}")

if __name__ == "__main__":
    visualize_filtered("output.ply", "reconstruction_fixed.html", "reconstruction_fixed.png")
