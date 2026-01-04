#pragma once

#include "engine/geometry/primitives.hpp"
#include "engine/mesh/mesh_data.hpp"
#include "render/renderer.hpp"

#include <cstdint>

class MeshFactory {
public:
  explicit MeshFactory(Renderer &renderer) : m_renderer(&renderer) {}

  void setRenderer(Renderer &renderer) noexcept { m_renderer = &renderer; }

  [[nodiscard]] MeshHandle cube(float size = 1.0F) const {
    return upload(engine::primitives::cube(size));
  }

  [[nodiscard]] MeshHandle square(float size = 1.0F) const {
    return upload(engine::primitives::triangle(size));
  }

  [[nodiscard]] MeshHandle triangle(float size = 1.0F) const {
    return upload(engine::primitives::triangle(size));
  }

  [[nodiscard]] MeshHandle circle(std::uint32_t segments,
                                  float radius = 0.5F) const {
    return upload(engine::primitives::circle(segments, radius));
  }

private:
  [[nodiscard]] MeshHandle upload(engine::MeshData &&cpu) const {
    if (m_renderer == nullptr) {
      return {};
    }

    return m_renderer->createMesh(cpu);
  }

  Renderer *m_renderer = nullptr; // non-owning
};
