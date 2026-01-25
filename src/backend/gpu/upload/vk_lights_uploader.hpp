#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "render/scene/lights_gpu.hpp"
#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>

struct PointLightUploadResult {
  uint32_t lightCount = 0;
  explicit operator bool() const noexcept { return lightCount != 0; }
};

namespace LightsUploader {

// Uploads the whole per-frame light list into that frame slice
PointLightUploadResult uploadPointLights(VkUploadContext::Recorder recorder,
                                         VkBuffer pointLightBuffer,
                                         VkDeviceSize frameBaseBytes,
                                         VkDeviceSize frameStrideBytes,
                                         uint32_t maxPointLightsPerFrame,
                                         std::span<const PointLightGPU> lights);

} // namespace LightsUploader
