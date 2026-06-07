import numpy as np
import matplotlib.pyplot as plt

# Parameters
r = 3.9             # Growth rate (try 2.5 to 4)
x0 = 0.5            # Initial value (between 0 and 1)
n_iter = 100        # Number of iterations

# Run the logistic map
x = np.zeros(n_iter)
x[0] = x0
for i in range(1, n_iter):
    x[i] = r * x[i - 1] * (1 - x[i - 1])

# Plot the result
plt.figure(figsize=(8, 4))
plt.plot(range(n_iter), x, marker='o', markersize=3, linestyle='-')
plt.title(f"Logistic Map (r = {r})")
plt.xlabel("Iteration (n)")
plt.ylabel("x[n]")
plt.grid(True)
plt.tight_layout()
plt.show()
