import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Parameters
m = 1.0          # mass
k = 1.0          # spring constant
x0 = 1.0         # initial displacement
v0 = 0.0         # initial velocity
dt = 0.02        # time step
total_time = 10  # total simulation time in seconds

# Initialize state variables
x = x0
v = v0

# Lists to store position over time (for plotting)
positions = []
times = np.arange(0, total_time, dt)

fig, ax = plt.subplots()
ax.set_xlim(-1.5, 1.5)
ax.set_ylim(-0.5, 0.5)
ax.set_xlabel('Position (m)')
ax.set_title('Simple Harmonic Motion: Mass on a Spring')

# The line will represent the spring's equilibrium line
line, = ax.plot([-1.5, 1.5], [0, 0], 'k-', lw=2)
# The dot represents the mass
mass_dot, = ax.plot([], [], 'ro', ms=15)

def update(frame):
    global x, v
    # Calculate acceleration
    a = -(k/m) * x
    # Update velocity and position using Euler method
    v += a * dt
    x += v * dt

    # Update the position of the mass dot
    mass_dot.set_data([x], [0])
    return mass_dot,

ani = animation.FuncAnimation(fig, update, frames=len(times),
                              interval=dt*1000, blit=True)

plt.show()


