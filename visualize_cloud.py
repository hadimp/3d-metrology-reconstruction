import sys
import argparse
import numpy as np

def read_ply(filename):
    points = []
    with open(filename, 'r') as f:
        lines = f.readlines()
        
    stride = 100 
    count = 0
    start_reading = False
    for line in lines:
        if "end_header" in line:
            start_reading = True
            continue
        if start_reading:
            count += 1
            if count % stride == 0:
                vals = line.strip().split()
                if len(vals) >= 3:
                    points.append([float(vals[0]), float(vals[1]), float(vals[2])])
    return np.array(points)

def visualize(filename, save_path=None, html_path=None):
    print(f"Loading {filename}...")
    points = read_ply(filename)
    
    # Filter Outliers (5th to 95th percentile)
    z_vals = points[:, 2]
    # Filter valid Z
    valid_mask = np.isfinite(z_vals)
    points = points[valid_mask]
    z_vals = points[:, 2]

    if len(z_vals) > 0:
        z_min = np.percentile(z_vals, 5)
        z_max = np.percentile(z_vals, 95)
        print(f"Filtering Z-range: {z_min:.2f} to {z_max:.2f}")
        mask = (z_vals > z_min) & (z_vals < z_max)
        points = points[mask]

    print(f"Visualizing {len(points)} points...")
    print("Coordinates sample:\n", points[:5])

    # Force Matplotlib for saving functionality consistency
    # (Open3D is harder to render off-screen without EGL)
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
    
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Subsample for plotting speed if still too large
    if len(points) > 20000:
        idx = np.random.choice(len(points), 20000, replace=False)
        plot_points = points[idx]
    else:
        plot_points = points
        
    # Use Z for color
    ax.scatter(plot_points[:,0], plot_points[:,1], plot_points[:,2], c=plot_points[:,2], cmap='viridis', marker='.', s=1)
    
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title(f"Reconstruction: {len(points)} points")
    
    # Set reasonable view
    ax.view_init(elev=-90, azim=-90) # Top-down view approx?
    
    if save_path:
        print(f"Saving visualization to {save_path}...")
        plt.savefig(save_path)
        print("Done.")
    elif html_path:
        print(f"Generating interactive HTML to {html_path}...")
        try:
            import plotly.graph_objects as go
            
            # Subsample for web performance (max ~50k points recommended for smooth interactivity)
            target_count = 50000
            if len(points) > target_count:
                print(f"Subsampling to {target_count} for HTML performance...")
                idx = np.random.choice(len(points), target_count, replace=False)
                plot_points = points[idx]
            else:
                plot_points = points

            fig = go.Figure(data=[go.Scatter3d(
                x=plot_points[:,0],
                y=plot_points[:,1],
                z=plot_points[:,2],
                mode='markers',
                marker=dict(
                    size=2,
                    color=plot_points[:,2],                # set color to an array/list of desired values
                    colorscale='Viridis',   # choose a colorscale
                    opacity=0.8
                )
            )])
            
            fig.update_layout(
                title=f"Reconstruction ({len(points)} total points)",
                scene=dict(
                    xaxis_title='X',
                    yaxis_title='Y',
                    zaxis_title='Z',
                    aspectmode='data' # Important to preserve 1:1:1 aspect ratio
                ),
                margin=dict(r=0, l=0, b=0, t=40)
            )
            
            fig.write_html(html_path)
            print("Done.")
            
        except ImportError:
            print("Error: Plotly not installed. Run 'pip install plotly'")
            
    else:
        plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Visualize PLY point cloud')
    parser.add_argument('filename', help='Path to PLY file')
    parser.add_argument('--save', help='Path to save PNG image', default=None)
    parser.add_argument('--html', help='Path to save interactive HTML', default=None)
    
    args = parser.parse_args()
    visualize(args.filename, args.save, args.html)
