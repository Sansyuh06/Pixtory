import numpy as np
from scipy.integrate import solve_ivp
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Lorenz system parameters
sigma = 10
rho = 60
beta = 8 / 3

# Lorenz system differential equations
def lorenz(t, state):
    x, y, z = state
    dxdt = sigma * (y - x)
    dydt = x * (rho - z) - y
    dzdt = x * y - beta * z
    return [dxdt, dydt, dzdt]

# Initial state
initial_state = [5.0, 1.0, 1.0]

# Time span
t_start = 0
t_end = 50
dt = 0.01
t_eval = np.arange(t_start, t_end, dt)

# Solve ODE
solution = solve_ivp(lorenz, (t_start, t_end), initial_state, t_eval=t_eval)

# Extract the solution
x = solution.y[0]
y = solution.y[1]
z = solution.y[2]

# Plot the Lorenz attractor
fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection='3d')
ax.plot(x, y, z, lw=0.5)
ax.set_title("Lorenz Attractor")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")
plt.tight_layout()
plt.show()
