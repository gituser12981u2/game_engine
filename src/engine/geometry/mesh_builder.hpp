#pragma once

#include "engine/mesh/mesh_data.hpp"
#include "engine/mesh/vertex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <utility>
#include <vector>
namespace engine {

class MeshBuilder {
public:
  std::vector<Vertex> vertices;
  std::vector<std::uint32_t> indices;

  void reserve(std::size_t vtx, std::size_t idx) {
    vertices.reserve(vtx);
    indices.reserve(idx);
  }

  static constexpr std::array<glm::vec2, 4> quadUvs01() noexcept {
    return {glm::vec2{0, 0}, glm::vec2{1, 0}, glm::vec2{1, 1}, glm::vec2{0, 1}};
  }

  std::uint32_t addVertex(const Vertex &v) {
    vertices.push_back(v);
    return static_cast<std::uint32_t>(vertices.size() - 1);
  }

  void addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
  }

  void addQuadIndices(std::uint32_t base) {
    // (0, 1, 2) (2, 3, 0)
    addTriangle(base + 0, base + 1, base + 2);
    addTriangle(base + 2, base + 3, base + 0);
  }

  void addQuad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
               glm::vec3 color,
               const std::array<glm::vec2, 4> &uvs = quadUvs01()) {
    const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());

    vertices.push_back(Vertex{.pos = a, .color = color, .uv = uvs[0]});
    vertices.push_back(Vertex{.pos = b, .color = color, .uv = uvs[1]});
    vertices.push_back(Vertex{.pos = c, .color = color, .uv = uvs[2]});
    vertices.push_back(Vertex{.pos = d, .color = color, .uv = uvs[3]});

    addQuadIndices(base);
  }

  MeshData build() && {
    MeshData out;
    out.vertices = std::move(vertices);
    out.indices = std::move(indices);
    return out;
  }
};

} // namespace engine
