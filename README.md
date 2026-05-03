# CRATER - Vulkan Rendering Engine

A modern 3D rendering engine built from scratch using Vulkan API, featuring Entity-Component-System architecture and advanced rendering techniques.

![CRATER Screenshot](C:\Users\karan\OneDrive\Pictures\Screenshots\Screenshot 2026-05-03 140717.png)
<!-- Add a screenshot of your rendered scene here -->

## Overview

CRATER is a personal rendering engine project focused on learning modern graphics programming practices and Vulkan API. The engine demonstrates real-time 3D rendering with a clean separation between the ECS layer and the rendering backend.

## Features

### Core Rendering
- **Multi-object rendering** with efficient batching and draw call optimization
- **Frequency-based descriptor set organization** (per-frame camera data, per-material textures)
- **Push constants** for per-object transformations
- **Depth buffering** with proper Z-testing
- **Texture mapping** with support for multiple texture formats

### Architecture
- **ECS-based scene management** using EnTT library
- **Resource Manager pattern** decoupling ECS from renderer
- **SPIR-V reflection** for automatic shader binding discovery
- **RAII-based Vulkan wrapper** for safe resource management

### Technical Highlights
- **Vulkan Memory Allocator (VMA)** integration for efficient GPU memory management
- **Custom vertex/index buffer abstractions** with shared vertex layout
- **FPS camera system** with WASD movement and mouse look (SDL3)
- **Multi-stage push constant reflection** with validation across vertex/fragment shaders
- **Swapchain management** with proper synchronization primitives

## Tech Stack

- **Graphics API:** Vulkan 1.3
- **Language:** C++20
- **Window/Input:** SDL3
- **ECS:** EnTT
- **Memory Management:** Vulkan Memory Allocator (VMA)
- **Math:** GLM
- **Model Loading:** tinyobjloader
- **Build System:** CMake
- **Package Manager:** vcpkg

## Prerequisites

- **C++20 compatible compiler** (MSVC 2022, GCC 11+, or Clang 14+)
- **Vulkan SDK** 1.3 or later ([Download here](https://vulkan.lunarg.com/))
- **CMake** 3.20 or later
- **vcpkg** package manager
- **Git**

## Architecture Overview

### Rendering Pipeline
1. **Scene** owns EnTT registry and systems
2. **ResourceManager** manages GPU resources (meshes, textures, materials)
3. **Renderer** consumes RenderObjects from ResourceManager
4. **RenderObjects** are cached in a vector, sorted by material ID
5. Draw calls are batched per material to minimize state changes

### Descriptor Set Layout
- **Set 0:** Per-frame data (camera UBO)
- **Set 1:** Per-material data (textures, material properties)
- **Push Constants:** Per-object model matrix

## Roadmap

- [ ] PBR (Physically Based Rendering) materials
- [ ] Shadow mapping (directional + point lights)
- [ ] Deferred rendering pipeline
- [ ] SSAO (Screen-Space Ambient Occlusion)
- [ ] Bloom post-processing
- [ ] ImGui integration for debug UI
- [ ] Scene serialization/deserialization
- [ ] Multi-threaded command buffer recording

## Known Issues

- Validation layers may report warnings about descriptor set layout optimization
- Hot-reload of shaders not yet implemented
- Limited error handling for missing assets