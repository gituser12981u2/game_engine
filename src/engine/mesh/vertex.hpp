#pragma once

#include "glm/ext/vector_float3.hpp"

namespace engine {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
};

static_assert(sizeof(Vertex) == sizeof(float) * 6);

} // namespace engine
