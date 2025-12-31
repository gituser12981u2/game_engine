#pragma once

#include "backend/render/renderer.hpp"
#include "engine/assets/gltf/gltf_types.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <string>

namespace engine::assets {

struct GltfAsset {
  std::vector<DrawItem> drawItems;
  glm::mat4 root = glm::mat4(1.0F);
};

bool loadGltf(Renderer &renderer, const std::string &path, GltfAsset &out,
              const GltfLoadOptions &options = {});

} // namespace engine::assets
