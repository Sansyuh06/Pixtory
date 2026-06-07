import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d import Axes3D

# --- Rossler Parameters ---
a, b, c = 0.2, 0.2, 5.7
dt = 0.02
total_steps = 4000

# --- State Variables ---
x, y, z = [0.1], [0.0], [0.0]

# --- Simulation Logic (RK4 Integration) ---
def rossler_deriv(curr_x, curr_y, curr_z):
    dx = -(curr_y + curr_z)
    dy = curr_x + a * curr_y
    dz = b + curr_z * (curr_x - c)
    return dx, dy, dz

print("🌌 Generating Rossler Attractor path...")
for _ in range(total_steps):
    dx1, dy1, dz1 = rossler_deriv(x[-1], y[-1], z[-1])
    dx2, dy2, dz2 = rossler_deriv(x[-1] + dx1*dt/2, y[-1] + dy1*dt/2, z[-1] + dz1*dt/2)
    dx3, dy3, dz3 = rossler_deriv(x[-1] + dx2*dt/2, y[-1] + dy2*dt/2, z[-1] + dz2*dt/2)
    dx4, dy4, dz4 = rossler_deriv(x[-1] + dx3*dt, y[-1] + dy3*dt, z[-1] + dz3*dt)
    
    x.append(x[-1] + (dt/6)*(dx1 + 2*dx2 + 2*dx3 + dx4))
    y.append(y[-1] + (dt/6)*(dy1 + 2*dy2 + 2*dy3 + dy4))
    z.append(z[-1] + (dt/6)*(dz1 + 2*dz2 + 2*dz3 + dz4))

x, y, z = np.array(x), np.array(y), np.array(z)

# --- Visualization Setup ---
plt.style.use('dark_background')
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')
ax.set_facecolor('black')
fig.patch.set_facecolor('black')

# Style the grid
ax.grid(False)
ax.xaxis.pane.fill = False
ax.yaxis.pane.fill = False
ax.zaxis.pane.fill = False
ax.set_axis_off()

# Plot elements
line, = ax.plot([], [], [], color='#ff4400', lw=1.5, alpha=0.8)
point, = ax.plot([], [], [], 'ro', markersize=4, markeredgecolor='white', label='Particle')
text = ax.text2D(0.05, 0.95, "Phase: Tracing", transform=ax.transAxes, color='white', fontsize=12)

# Set limits
ax.set_xlim(np.min(x), np.max(x))
ax.set_ylim(np.min(y), np.max(y))
ax.set_zlim(np.min(z), np.max(z))

# Animation Update Function
def update(i):
    # Phase 1 & 2: Tracing and Attractor Buildup
    if i < total_steps:
        line.set_data(x[:i], y[:i])
        line.set_3d_properties(z[:i])
        point.set_data(x[i:i+1], y[i:i+1])
        point.set_3d_properties(z[i:i+1])
        text.set_text(f"Phase: Rossler Attractor\nParticles: {i}")
        line.set_color('#ff5500')
    
    # Phase 3: Black Hole Collapse
    else:
        progress = (i - total_steps) / 100.0
        collapse_x = x * (1 - progress)
        collapse_y = y * (1 - progress)
        collapse_z = z * (1 - progress)
        
        line.set_data(collapse_x, collapse_y)
        line.set_3d_properties(collapse_z)
        line.set_color('#ff0000')
        line.set_alpha(max(0, 0.8 - progress))
        
        point.set_data([0], [0])
        point.set_3d_properties([0])
        point.set_markersize(20 * progress)
        point.set_color('white')
        
        text.set_text("Phase: BLACK HOLE COLLAPSE")
        text.set_color('red')

    # Slow rotation
    ax.view_init(elev=20, azim=i*0.2)
    return line, point, text

# Launch Animation
print("🚀 Launching Animation...")
ani = FuncAnimation(fig, update, frames=total_steps + 100, interval=1, blit=False)
plt.title("Rössler Attractor → Black Hole Simulation", color='white', pad=20)
plt.show()
