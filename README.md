# Graphic-Computing

> Developed for the Bachelor's Degree in Computer Science:
> - University: Universidade do Vale do Rio dos Sinos (Unisinos)
> - Subject: Graphic Computing
> - Semester: 2026/01

# Objective

This repository contains the projects developed during the **Graphic Computing** course. All projects are implemented in **C++ using OpenGL**, focusing on the fundamentals of computer graphics such as 2D/3D rendering, transformations, and camera manipulation.

# How to Run
Refer to file /how-to-run.txt

## 1. Triangle
Initial implementation of a **2D triangle** rendered using OpenGL. This project introduces the graphics pipeline and basic rendering setup.

## 2. Hello3D and HelloCamera
This project introduces **3D rendering** with a traingle, and demonstrates the use of 3D transformations and basic camera concepts. It has the below features:

- Keyboard controls for rotation:
  - **X** → Rotate around X axis  
  - **Y** → Rotate around Y axis  
  - **Z** → Rotate around Z axis  

## 3. HelloSinteticCamera
Extends the previous project by adding **camera movement and depth control**, with the following features:
- 3D triangle rendered in space
- Rotation using:
  - **X / Y / Z** → Rotate the object around the mentioned axis  
- Depth movement:
  - **W / A / S / D** → Move the object closer/farther from the camera

## 4. ObjectExercise
This program builds a simple **scene manager** capable of loading, storing, rendering and selecting multiple objects. Features:
 - **W / A / S / D** → Move the object closer/farther from the camera
- **X / Y / Z** → Rotate the object around the mentioned axis  
- **TAB** → Change the selected object

## 5. ReaderViewer3D
Enhance ObjectExercise to support interactive 3D transformations on multiple objects in the scene.
Features:
 - **1 / 2 / 3** → Change the mode (1) Translation (2) Translation (3) Scale
- **W/S or ↑/↓** → Rotate the object around the Y axis
- **A/D or ←/→** → Rotate the object around the X axis
- **Q / E** → Rotate the object around the Z axis
- **+/-** → Uniform scale (mode 3)
- **P** → Perspective / orthographic
- **TAB** → Change the selected object