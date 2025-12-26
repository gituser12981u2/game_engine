#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

namespace engine {

struct Vertex {
  glm::vec3 pos{0.0F};
  glm::vec3 color{1.0F};
  glm::vec2 uv{0.0F};
};

static_assert(sizeof(Vertex) == sizeof(float) * 8);

} // namespace engine
