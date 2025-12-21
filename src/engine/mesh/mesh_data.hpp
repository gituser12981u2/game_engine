#pragma once

#include "../mesh/vertex.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace engine {

struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

} // namespace engine
