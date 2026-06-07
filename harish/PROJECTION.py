import numpy as np
import matplotlib.pyplot as plt

# Parameters
v0 = 30             # Initial speed (m/s)
theta = 45          # Launch angle (degrees)
g = 9.81            # Acceleration due to gravity (m/s^2)

# Convert angle to radians
theta_rad = np.radians(theta)

# Time of flight
t_flight = 2 * v0 * np.sin(theta_rad) / g

# Time points
t = np.linspace(0, t_flight, num=500)

# Position calculations
x = v0 * np.cos(theta_rad) * t
y = v0 * np.sin(theta_rad) * t - 0.5 * g * t**2

# Plot trajectory
plt.figure(figsize=(8, 5))
plt.plot(x, y)
plt.title("Projectile Motion")
plt.xlabel("Distance (m)")
plt.ylabel("Height (m)")
plt.grid(True)
plt.ylim(bottom=0)
plt.show()
