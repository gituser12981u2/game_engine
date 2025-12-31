#include "engine/assets/gltf/gltf_asset.hpp"
#include "backend/render/renderer.hpp"
#include "engine/assets/gltf/gltf_cpu_loader.hpp"
#include "engine/assets/gltf/gltf_gpu_builder.hpp"
#include "engine/assets/gltf/gltf_types.hpp"

#include <glm/ext/matrix_float4x4.hpp>

namespace engine::assets {

bool loadGltf(Renderer &renderer, const std::string &path, GltfAsset &out,
              const GltfLoadOptions &options) {
  out = {};

  GltfSceneCpu cpu{};
  if (!loadGltfCpu(path, cpu, options)) {
    return false;
  }

  GltfSceneGpu gpu{};
  if (!buildGltfSceneGpu(renderer, path, cpu, gpu)) {
    return false;
  }

  out.drawItems = std::move(gpu.drawItems);
  out.root = glm::mat4(1.0F);

  return !out.drawItems.empty();
}

} // namespace engine::assets
