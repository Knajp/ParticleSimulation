# Particle Simulation

A GPU-accelerated particle simulation built with C++ and Vulkan. Spawns about 2000 particles all over the screen in an uniform grid, then applies gravity and collisions. Renders and simulates 100'000 particles at 120 FPS on RX 9070 XT.

## Todo
Add mouse interactions.

## Features
- Compute shader-based particle physics
- Gravity and wall collisions
- Particle-to-particle collision detection
- Uniform grid with linked lists forneighbor searches

## Tech Stack
- C++
- Vulkan
- GLSL 450
- CMake

## Launch Instructions
**Linux Only**
Download particleSimulation.tar.gz from the Releases page, unpack it, double click the .AppImage file.
