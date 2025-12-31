#pragma once

#include "engine/mesh/mesh_data.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

namespace engine::assets {

struct GltfMaterialCpu {
  // TODO: add more than albedo
  std::string baseColorTextureUri;

  // gLTF baseColorFactor in (RGBA)
  glm::vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
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
