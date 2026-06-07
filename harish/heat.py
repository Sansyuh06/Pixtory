import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Simulation parameters
nx = 100               # number of spatial points
dx = 1.0               # spatial step size
dt = 0.1               # time step size
kappa = 1.0            # thermal diffusivity
alpha = kappa * dt / dx**2

# Stability check
if alpha > 0.5:
    print("Warning: alpha > 0.5 may cause instability!")

# Initial temperature distribution
u = np.zeros(nx)
u[nx//2] = 100.0  # Heat pulse in the center

# Prepare for plotting
fig, ax = plt.subplots()
line, = ax.plot(u)
ax.set_ylim(0, 120)
ax.set_title("1D Heat Diffusion")

# Function to update each frame
def update(frame):
    global u
    u_new = u.copy()
    for i in range(1, nx - 1):
        u_new[i] = u[i] + alpha * (u[i+1] - 2*u[i] + u[i-1])
    u = u_new
    line.set_ydata(u)
    return line,

# Animation
ani = animation.FuncAnimation(fig, update, frames=200, interval=50)
plt.show()
