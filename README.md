# Vulkan VFX

## Introduction
Welcome to Vulkan VFX, a visual effects platform library harnessing the power of the Vulkan API to deliver high-performance, real-time graphics rendering. Inspired by the platform ShaderToy, this platform can be used to write and test your
Vulkan VFX pixel shader effects and particle systems. Benchmarking these shaders both in terms of performance and visual quality will also be a critical aspect of this project. This platform will also be able to support very large levels and
worlds such that the natural movement of an avatar in a chosen virtual world can be visualized.

## How to Install and Run the Project
The project can be cloned and opened on VS 2019+.

## How to Use the Project
Open in VS and run as debug.

## Current Features
* Deferred Rendering Pipeline.
* Multi threaded frustum culling algorithm to prune large sections of geometry per frame.
* Shadow Mapping.
* ImGUI.
* Hybrid Ray-tracing and Frustum rendering engine.

## Needed Features
* Particle systems.
* Non-particle pixel shader effects.
* More detailed Shadow Mapping ( Cascading shadow maps ).
* Implementation of Low Poly geometry for distance rendering.
* Ray-traced shadows.

## How to Contribute to the Project

1. Fork the project.
2. Create your feature branch (git checkout -b feature/AmazingFeature).
3. Commit your changes (git commit -m 'Add some AmazingFeature').
4. Push to the branch (git push origin feature/AmazingFeature).
5. Open a pull request.
