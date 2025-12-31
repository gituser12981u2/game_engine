#pragma once

#include "backend/render/renderer.hpp"
#include "engine/assets/gltf/gltf_types.hpp"

namespace engine::assets {

struct GltfSceneGpu {
  std::vector<MeshHandle>
      primitiveMeshes;               // index = GltfSceneCpu::primitives index
  std::vector<uint32_t> materialIds; // index = GltfSceneCpu::materials index
  std::vector<DrawItem> drawItems;   // one per node primitive instance
};

struct GltfBuildOptions {
  bool flipTextureY = true;
  bool preferTextureOverFactor = true;
};

bool buildGltfSceneGpu(Renderer &renderer, const std::string &gltfPath,
                       const engine::assets::GltfSceneCpu &cpu,
                       GltfSceneGpu &outGpu,
                       const GltfBuildOptions &options = {});

} // namespace engine::assets
