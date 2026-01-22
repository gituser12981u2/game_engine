# Work in Progress Game Engine for Quake-Style Game

This engine shall be written from scratch in Vulkan, targeting macOS, Windows, and Linux. This only supports devices 
with Vulkan 1.3+ or above, for dynamic rendering mainly.

The engine shall support
- Custom Physics Engine
- PBR
- Animations
- Water Rendering
- Multiple Pipelines
- GUI Editor

Advanced possible supports
- Virtualized Geometry 
- Global Illumination

The game that shall be made with this engine is a game with movement inspired by quake.
The game shall feature older-quake-like graphics and will likely have a single player,
multiplayer, and a COD zombies inspired mode.

The focus of this game engine is to make a game playable on all devices with little hassle.

## MVP 1

- [x] Arbitrary 3d Meshes (2025-12-19)
- [x] Moveable Camera
- [x] Textures
    - [x] Albedos (start of PBR)
    - [x] Diffuse option
- [x] material SSBO (PBR) 
- [ ] gITF imports
    - [x] mesh import
    - [x] base texture/material
    - [ ] normals
    - [x] other PBR
- [ ] lighting
- [x] PBR API for non imports 
- [x] Vulkan Memory Allocator (VMA) refactor 
- [ ] Mipmaps
- [ ] Depth pre-pass 
- [ ] Forward+ lighting

## MVP 2
- [ ] Editor
- [ ] Job System
    - Basic Multithreading for uploader [x]
    - Dynamic picking of threads for jobs [ ]
- [ ] ECS

- [ ] Basic physics engine
- [ ] Player Controller
