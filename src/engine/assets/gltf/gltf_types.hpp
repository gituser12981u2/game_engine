#pragma once

#include "engine/mesh/mesh_data.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

namespace engine::assets {

enum class GltfAlphaMode : uint8_t { Opaque = 0, Mask = 1, Blend = 2 };

struct GltfMaterialCpu {
  std::string baseColorTextureUri;
  std::string metallicRoughnessTextureUri;
  std::string occlusionTextureUri;
  std::string emissiveTextureUri;
  std::string normalTextureUri;

  glm::vec4 baseColorFactor{1, 1, 1, 1};
  glm::vec3 emissiveFactor{0, 0, 0};
  float metallicFactor = 1.0F;
  float roughnessFactor = 1.0F;
  float occlusionStrength = 1.0F;

  GltfAlphaMode alphaMode = GltfAlphaMode::Opaque;
  float alphaCutoff = 0.5F;
  bool doubleSided = false;
};

struct GltfPrimitiveCpu {
  engine::MeshData mesh;
  std::uint32_t materialIndex = UINT32_MAX;
};

struct GltfNodeCpu {
  glm::mat4 model = glm::mat4(1.0F);
  // TODO: vector of primitives or mesh index + primitive list
  std::uint32_t primitiveIndex = UINT32_MAX;
};

struct GltfSceneCpu {
  std::vector<GltfMaterialCpu> materials;
  std::vector<GltfPrimitiveCpu> primitives;
  std::vector<GltfNodeCpu> nodes;
};

struct GltfAxisOptions {
  // gLTF is typically RH, +Y up. Map to +Z up.
  bool yUpToZUp = true;

  // If -Z in gLTF is +Z in engine
  bool flipAxisZ = false;
};

struct GltfLoadOptions {
  bool flipTexcoordV = true;
  bool requireTexcoord0 = false;

  GltfAxisOptions axis{};
};

} // namespace engine::assets
