#pragma once

#include "engine/assets/gltf/gltf_types.hpp"

#include <glm/ext/matrix_float4x4.hpp>

namespace engine::assets {

bool loadGltfCpu(const std::string &gltfPath, GltfSceneCpu &out,
                 const GltfLoadOptions &opt = {});

} // namespace engine::assets
