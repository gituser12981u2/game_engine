#pragma once

#include "engine/mesh/mesh_data.hpp"

namespace engine::primitives {

MeshData triangle(float size = 1.0F);
MeshData square(float size = 1.0F);
MeshData cube(float size = 1.0F);
MeshData circle(uint32_t segments, float radius = 0.5F);
MeshData poincareDisk();

} // namespace engine::primitives
