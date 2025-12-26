#include "primitives.hpp"
#include "engine/geometry/mesh_builder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <numbers>
#include <utility>

namespace engine::primitives {

MeshData triangle(float size) {
  engine::MeshBuilder builder;
  builder.reserve(3, 0);

  const float h = size * 0.5F;

  builder.vertices.push_back(engine::Vertex{
      .pos = {0.0F, -h, 0.0F},
      .color = {1.0F, 1.0F, 0.0F},
      .uv = {0.5F, 0.0F},
  });

  builder.vertices.push_back(engine::Vertex{
      .pos = {+h, +h, 0.0F},
      .color = {1.0F, 0.0F, 1.0F},
      .uv = {1.0F, 1.0F},
  });

  builder.vertices.push_back(engine::Vertex{
      .pos = {-h, +h, 0.0F},
      .color = {0.0F, 1.0F, 1.0F},
      .uv = {0.0F, 1.0F},
  });

  return std::move(builder).build();
}

MeshData square(float size) {
  engine::MeshBuilder builder;
  builder.reserve(4, 6);

  const float h = size * 0.5F;

  builder.addQuad({-h, -h, 0.0F}, {+h, -h, 0.0F}, {+h, +h, 0.0F},
                  {-h, +h, 0.0F}, {1.0F, 1.0F, 1.0F});

  return std::move(builder).build();
}

MeshData cube(float size) {
  engine::MeshBuilder builder;
  builder.reserve(24, 36);

  const float h = size * 0.5F;

  // +Z (front)
  builder.addQuad({-h, -h, +h}, {+h, -h, +h}, {+h, +h, +h}, {-h, +h, +h},
                  {1, 0, 0});
  // -Z (back)
  builder.addQuad({+h, -h, -h}, {-h, -h, -h}, {-h, +h, -h}, {+h, +h, -h},
                  {0, 1, 0});
  // -X (left)
  builder.addQuad({-h, -h, -h}, {-h, -h, +h}, {-h, +h, +h}, {-h, +h, -h},
                  {0, 0, 1});
  // +X (right)
  builder.addQuad({+h, -h, +h}, {+h, -h, -h}, {+h, +h, -h}, {+h, +h, +h},
                  {1, 1, 0});
  // +Y (top)
  builder.addQuad({-h, +h, +h}, {+h, +h, +h}, {+h, +h, -h}, {-h, +h, -h},
                  {1, 0, 1});
  // -Y (bottom)
  builder.addQuad({-h, -h, -h}, {+h, -h, -h}, {+h, -h, +h}, {-h, -h, +h},
                  {0, 1, 1});

  return std::move(builder).build();
}

MeshData circle(uint32_t segments, float radius) {

  if (radius == 0) {
    std::cerr << "[Primitives] circle cannot had radius of 0\n";
    return {};
  }

  engine::MeshBuilder builder;

  segments = std::max(segments, 3U);

  builder.reserve(static_cast<size_t>(segments) + 2U,
                  static_cast<size_t>(segments) * 3U);

  // center
  builder.vertices.push_back(engine::Vertex{
      .pos = glm::vec3{0.0F, 0.0F, 0.0F},
      .color = glm::vec3{1.0F, 1.0F, 1.0F},
      .uv = {0.5F, 0.5F},
  });

  // ring
  for (std::uint32_t i = 0; i <= segments; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(segments);
    const float a = t * 2.0F * std::numbers::pi_v<float>;

    const float x = std::cos(a) * radius;
    const float y = std::sin(a) * radius;

    // map [-radius, radius] -> [0, 1]
    const glm::vec2 uv{(x / (2.0F * radius)) + 0.5F,
                       (y / (2.0F * radius)) + 0.5F};

    builder.vertices.push_back(engine::Vertex{
        .pos = glm::vec3{x, y, 0.0F},
        .color = glm::vec3{1.0F, 0.0F, 0.0F},
        .uv = uv,
    });
  }

  // indices
  for (std::uint32_t i = 1; i <= segments; ++i) {
    builder.indices.push_back(0U);
    builder.indices.push_back(i);
    builder.indices.push_back(i + 1U);
  }

  return std::move(builder).build();
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
