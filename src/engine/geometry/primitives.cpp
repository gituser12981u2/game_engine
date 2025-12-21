#include "primitives.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace engine::primitives {

MeshData triangle() {
  MeshData m;
  m.vertices = {
      engine::Vertex{.pos = glm::vec3{0.0F, -0.5F, 0.0F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, 0.0F},
                     .color = glm::vec3{1.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, 0.0F},
                     .color = glm::vec3{0.0F, 1.0F, 1.0F}},
  };

  // non-indexed
  return m;
}

MeshData square() {
  MeshData m;
  m.vertices = {
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, 0.0F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, 0.0F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, 0.0F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, 0.0F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
  };

  m.indices = {0U, 1U, 2U, 2U, 3U, 0U};
  return m;
}

MeshData cube() {
  MeshData m;
  m.vertices = {
      // +Z (front)
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},

      // -Z (back)
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},

      // -X (left)
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},

      // +X (right)
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},

      // +Y (top)
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, 0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, 0.5F, -0.5F},
                     .color = glm::vec3{1.0F, 0.0F, 1.0F}},

      // -Y (bottom)
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, -0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.5F, -0.5F, 0.5F},
                     .color = glm::vec3{0.0F, 1.0F, 1.0F}},
  };

  m.indices = {
      // front
      0U,
      1U,
      2U,
      2U,
      3U,
      0U,
      // back
      4U,
      5U,
      6U,
      6U,
      7U,
      4U,
      // left
      8U,
      9U,
      10U,
      10U,
      11U,
      8U,
      // right
      12U,
      13U,
      14U,
      14U,
      15U,
      12U,
      // top
      16U,
      17U,
      18U,
      18U,
      19U,
      16U,
      // bottom
      20U,
      21U,
      22U,
      22U,
      23U,
      20U,
  };

  return m;
}

MeshData circle(uint32_t segments) {
  MeshData m;

  segments = std::max(segments, 3U);

  m.vertices.reserve(static_cast<size_t>(segments) + 2U);
  m.indices.reserve(static_cast<size_t>(segments) * 3U);

  // center
  m.vertices.push_back(engine::Vertex{
      .pos = glm::vec3{0.0F, 0.0F, 0.0F},
      .color = glm::vec3{1.0F, 1.0F, 1.0F},
  });

  // ring
  for (uint32_t i = 0; i <= segments; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments);
    const float a = t * 2.0F * std::numbers::pi_v<float>;
    const float x = std::cos(a) * 0.5F;
    const float y = std::sin(a) * 0.5F;

    m.vertices.push_back(engine::Vertex{
        .pos = glm::vec3{x, y, 0.0F},
        .color = glm::vec3{1.0F, 0.0F, 0.0F},
    });
  }

  for (uint32_t i = 1; i <= segments; ++i) {
    m.indices.push_back(0U);
    m.indices.push_back(i);
    m.indices.push_back(i + 1U);
  }

  return m;
}

MeshData poincareDisk() {
  MeshData m;
  m.vertices = {
      // Bottom arc (curving inward)
      engine::Vertex{.pos = glm::vec3{-0.6F, -0.4F, 0.0F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.6F, -0.4F, 0.0F},
                     .color = glm::vec3{1.0F, 0.0F, 0.0F}},

      // Right arc
      engine::Vertex{.pos = glm::vec3{0.8F, -0.1F, 0.0F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{0.8F, 0.1F, 0.0F},
                     .color = glm::vec3{0.0F, 1.0F, 0.0F}},

      // Top arc
      engine::Vertex{.pos = glm::vec3{0.6F, 0.4F, 0.0F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},
      engine::Vertex{.pos = glm::vec3{-0.6F, 0.4F, 0.0F},
                     .color = glm::vec3{0.0F, 0.0F, 1.0F}},

      // Left arc
      engine::Vertex{.pos = glm::vec3{-0.8F, 0.1F, 0.0F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
      engine::Vertex{.pos = glm::vec3{-0.8F, -0.1F, 0.0F},
                     .color = glm::vec3{1.0F, 1.0F, 0.0F}},
  };

  m.indices = {0U, 1U, 2U, 2U, 3U, 4U, 4U, 5U, 6U,
               6U, 7U, 0U, 0U, 2U, 4U, 4U, 6U, 0U};

  return m;
}

} // namespace engine::primitives
