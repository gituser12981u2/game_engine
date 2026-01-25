#include "backend/gpu/upload/vk_lights_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/profilers/upload_profiler.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"
#include "render/scene/lights_gpu.hpp"
#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace LightsUploader {

PointLightUploadResult
uploadPointLights(VkUploadContext::Recorder recorder, VkBuffer pointLightBuffer,
                  VkDeviceSize frameBaseBytes, VkDeviceSize frameStrideBytes,
                  uint32_t maxPointLightsPerFrame,
                  std::span<const PointLightGPU> lights) {
  PointLightUploadResult out{};
  if (!recorder || pointLightBuffer == VK_NULL_HANDLE) {
    return out;
  }

  const uint32_t wanted = static_cast<uint32_t>(lights.size());
  const uint32_t count = std::min(wanted, maxPointLightsPerFrame);
  out.lightCount = count;

  const VkDeviceSize bytes = VkDeviceSize(count) * sizeof(PointLightGPU);
  if (bytes == 0) {
    return out;
  }

  if (bytes > frameStrideBytes) {
    LOGE("Point light SSBO descriptor range exceeded (bytes={}, stride={})",
         static_cast<uint64_t>(bytes), static_cast<uint64_t>(frameStrideBytes));
    out.lightCount = 0;
    return out;
  }

  VkStagingAlloc stageAlloc = recorder.allocStaging(bytes, /*alignment=*/16);
  if (!stageAlloc) {
    LOGE("allocStaging failed for point lights");
    out.lightCount = 0;
    return out;
  }

  std::memcpy(stageAlloc.ptr, lights.data(), static_cast<size_t>(bytes));

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadMemcpyCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::UploadMemcpyBytes, bytes);

  const VkDeviceSize dstOffset = frameBaseBytes;

  recorder.cmdCopyToBuffer(pointLightBuffer, dstOffset, stageAlloc.offset,
                           bytes);
  recorder.cmdBarrierBufferTransferToShader(
      pointLightBuffer, dstOffset, bytes,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

  // PROFILE_UPLOAD_INC(UploadProfiler::Stat::LightUploadCount);
  // PROFILE_UPLOAD_ADD(UploadProfiler::Stat::LightUploadBytes, bytes);

  return out;
}

} // namespace LightsUploader
