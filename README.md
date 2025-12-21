# Work in Progress Game Engine for Quake-Style Game

This engine shall be written from scratch in Vulkan, targeting macOS, Windows, and Linux.

The engine shall support
- Custom Physics Engine
- PBR
- Animations
- Water Rendering
- Multiple Pipelines
- GUI Editor?

The game that shall be made with this engine is a game with movement inspired by quake.
The game shall feature older-quake-like graphics and will likely have a single player,
multiplayer, and a COD zombies inspired mode.

The focus of this game engine is to make a game playable on all devices with little hassle.

## MVP 1

- [x] Arbitrary 3d Meshes (2025-12-19)
- [x] Moveable Camera
- [ ] Textures
    - [ ] Albedos (start of PBR)
    - [ ] Diffuse option
- [ ] gITF imports
- [ ] Lighting
    - [ ] Normals (PBR)
- [ ] Vulkan Memory Allocator (VMA) refactor 
- [ ] Mipmaps
- [ ] Basic physics engine
- [ ] Player Controller
