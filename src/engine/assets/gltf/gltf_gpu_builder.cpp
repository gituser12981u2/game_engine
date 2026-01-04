#include "engine/assets/gltf/gltf_gpu_builder.hpp"

#include "engine/assets/gltf/gltf_path.hpp"
#include "render/resources/material_gpu.hpp"
#include "render/resources/material_system.hpp"

#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <iostream>
#include <string>

namespace engine::assets {

bool buildGltfSceneGpu(Renderer &renderer, const std::string &gltfPath,
                       const GltfSceneCpu &cpu, GltfSceneGpu &outGpu,
                       const GltfBuildOptions &options) {
  outGpu = {};
  outGpu.materialIds.resize(cpu.materials.size(), UINT32_MAX);

  // TODO: cache textures by resolved path
  std::unordered_map<std::string, TextureHandle> texCache;

  for (size_t materialIdx = 0; materialIdx < cpu.materials.size();
       ++materialIdx) {
    const auto &m = cpu.materials[materialIdx];

    uint32_t matId = UINT32_MAX;

    if (!m.baseColorTextureUri.empty()) {
      const std::string texPath =
          resolveUriRelativeToFile(gltfPath, m.baseColorTextureUri);

      std::cout << "baseColorTextureUri found\n";

      TextureHandle texHandle{UINT32_MAX};
      if (auto item = texCache.find(texPath); item != texCache.end()) {
        texHandle = item->second;
      } else {
        texHandle =
            renderer.createTextureFromFile(texPath, options.flipTextureY);
        if (texHandle.id != UINT32_MAX) {
          texCache.emplace(texPath, texHandle);
        }
      }

      if (texHandle.id != UINT32_MAX) {
        matId = renderer.createMaterialFromTexture(texHandle);

        // set factor (texture * factor)
        if (matId != UINT32_MAX) {
          MaterialGPU gpu{};
          gpu.baseColorFactor = m.baseColorFactor;
          (void)renderer.updateMaterialGPU(matId, gpu);
        }
      }
    } else {
      matId = renderer.createMaterialFromBaseColorFactor(m.baseColorFactor);
    }

    outGpu.materialIds[materialIdx] = matId;
  }

  outGpu.primitiveMeshes.resize(cpu.primitives.size());
  for (size_t primitiveIdx = 0; primitiveIdx < cpu.primitives.size();
       ++primitiveIdx) {
    outGpu.primitiveMeshes[primitiveIdx] =
        renderer.createMesh(cpu.primitives[primitiveIdx].mesh);
  }

  outGpu.drawItems.reserve(cpu.nodes.size());
  for (const auto &node : cpu.nodes) {
    if (node.primitiveIndex == UINT32_MAX ||
        node.primitiveIndex >= outGpu.primitiveMeshes.size()) {
      continue;
    }

    const auto &primitiveCpu = cpu.primitives[node.primitiveIndex];

    uint32_t mat = UINT32_MAX;
    if (primitiveCpu.materialIndex != UINT32_MAX &&
        primitiveCpu.materialIndex < outGpu.materialIds.size()) {
      mat = outGpu.materialIds[primitiveCpu.materialIndex];
    }

    DrawItem drawItem{};
    drawItem.mesh = outGpu.primitiveMeshes[node.primitiveIndex];
    drawItem.material = mat;
    drawItem.model = node.model;
    outGpu.drawItems.push_back(drawItem);
  }

  return !outGpu.drawItems.empty();
}

} // namespace engine::assets
