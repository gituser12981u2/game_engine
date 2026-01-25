#include "engine/assets/gltf/gltf_gpu_builder.hpp"

#include "engine/assets/gltf/gltf_path.hpp"
#include "render/resources/material_system.hpp"

#include <cstdint>
#include <functional>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <utility>

namespace engine::assets {

namespace {

struct TexCacheKey {
  std::string path;
  MaterialSystem::TextureUsage usage;

  bool operator==(const TexCacheKey &other) const noexcept {
    return usage == other.usage && path == other.path;
  }
};

struct TexCacheKeyHash {
  size_t operator()(const TexCacheKey &key) const noexcept {
    std::hash<std::string> hs;
    std::hash<int> hi;
    size_t h = hs(key.path);
    h ^= (hi(static_cast<int>(key.usage)) + 0x9e3779b97f4a7c15ULL + (h << 6) +
          (h >> 2));
    return h;
  }
};

} // namespace

bool buildGltfSceneGpu(Renderer &renderer, const std::string &gltfPath,
                       const GltfSceneCpu &cpu, GltfSceneGpu &outGpu,
                       const GltfBuildOptions &options) {
  outGpu = {};
  outGpu.materialIds.resize(cpu.materials.size(), UINT32_MAX);

  std::unordered_map<TexCacheKey, TextureHandle, TexCacheKeyHash> texCache;

  auto loadTexCached =
      [&](const std::string &uri,
          MaterialSystem::TextureUsage usage) -> std::optional<TextureHandle> {
    if (uri.empty()) {
      return std::nullopt;
    }

    const std::string texPath = resolveUriRelativeToFile(gltfPath, uri);
    TexCacheKey key{.path = texPath, .usage = usage};

    if (auto it = texCache.find(key); it != texCache.end()) {
      if (it->second.id != UINT32_MAX) {
        return it->second;
      }

      return std::nullopt;
    }

    TextureHandle handle =
        renderer.loadTextureFromFile(texPath, options.flipTextureY, usage);

    texCache.emplace(std::move(key), handle);

    if (handle.id == UINT32_MAX) {
      return std::nullopt;
    }

    return handle;
  };

  for (size_t materialIdx = 0; materialIdx < cpu.materials.size();
       ++materialIdx) {
    const auto &m = cpu.materials[materialIdx];

    MaterialSystem::MaterialDescription desc{};
    desc.baseColorFactor = m.baseColorFactor;
    desc.emissiveFactor = m.emissiveFactor;

    desc.metallicFactor = m.metallicFactor;
    desc.roughnessFactor = m.roughnessFactor;
    desc.ambientOcclusionFactor = m.occlusionStrength;
    desc.alphaCutoff = m.alphaCutoff;

    desc.alphaMode = static_cast<uint32_t>(m.alphaMode);
    desc.doubleSided = m.doubleSided;

    // Textures
    desc.baseColorTexture = loadTexCached(m.baseColorTextureUri,
                                          MaterialSystem::TextureUsage::sRGB);

    desc.emissiveTexture =
        loadTexCached(m.emissiveTextureUri, MaterialSystem::TextureUsage::sRGB);

    desc.metallicRoughnessTexture = loadTexCached(
        m.metallicRoughnessTextureUri, MaterialSystem::TextureUsage::UNORM);

    desc.ambientOcclusionTexture = loadTexCached(
        m.occlusionTextureUri, MaterialSystem::TextureUsage::UNORM);

    desc.normal =
        loadTexCached(m.normalTextureUri, MaterialSystem::TextureUsage::UNORM);

    const uint32_t matId = renderer.createMaterial(desc);
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
