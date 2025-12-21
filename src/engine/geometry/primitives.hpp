#pragma once

#include "../mesh/mesh_data.hpp"

namespace engine::primitives {

MeshData triangle();
MeshData square();
MeshData cube();
MeshData circle(uint32_t segments);
MeshData poincareDisk();

} // namespace engine::primitives
