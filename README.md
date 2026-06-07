# Pixtory

## Problem Statement

Physics is often taught through equations and textbook diagrams, but many learners struggle to connect those abstractions to how motion and fields actually behave. This repository solves that gap by turning complex physics concepts into interactive, visual simulations that make pendulums, black holes, and chaotic systems easier to understand.

## Solution

`Pixtory` is a hands-on physics visualization project built with Python and Pygame. It uses real simulation techniques to render:
- a double pendulum with accurate Runge-Kutta integration,
- a stylized black hole accretion disk with gravitational lensing effects,
- and additional physics-driven visuals that emphasize physical intuition.

The result is not just a static demo: it is an exploratory learning environment where the code, visuals, and audio all support deeper understanding.

## Tech Stack

- Python 3
- Pygame for real-time rendering and input handling
- NumPy for numerical signal generation and performance-critical calculations
- Math and physics formulas for energy, orbital motion, and relativistic-style visual effects
- Threading for asynchronous audio playback while the visual simulation runs

## How This Project Was Built

1. **Identify the physics goals**
   - a double pendulum that exhibits chaotic motion,
   - a black hole with accretion disk and photon ring effects,
   - a visual interface that feels polished and cinematic.

2. **Model the system mathematically**
   - implement the pendulum equations of motion using Runge-Kutta 4 integration,
   - compute energy and state evolution for accurate simulation,
   - build a 3D projection and lensing approximation for the black hole scene.

3. **Build the renderer**
   - use Pygame to manage window, drawing, and event loops,
   - render motion trails, particle systems, and UI overlays,
   - add color and lighting choices that enhance the scientific storytelling.

4. **Add sensory feedback**
   - integrate procedural audio based on pendulum angular velocity,
   - use color gradients and particle glow to make dynamics readable.

5. **Iterate and polish**
   - improve visual clarity with consistent palettes,
   - add user-facing details like camera control, simulation speed, and energy tracking,
   - keep the code structured so the physics logic remains easy to extend.

## Impact

This repository makes physics concepts more accessible and enjoyable by:
- helping students and hobbyists see how equations translate into motion,
- providing an interactive, visual way to explore chaos and gravity,
- encouraging experimentation with parameters and initial conditions,
- turning a traditional learning experience into a creative simulation project.

## How to Use

1. Install Python 3.10+.
2. Install dependencies:
   ```bash
   pip install pygame numpy
   ```
3. Run one of the simulation files:
   ```bash
   python pendulum.py
   python blackhole.py
   ```

## Notes

This README introduces the project purpose, the physics it explores, and why this repo is designed as a visualization-first learning toolkit. If you want, I can also add a dedicated `requirements.txt` and a starter script for launching the main simulations.
