#include "backend/gpu/upload/vk_material_uploader.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace MaterialUploader {

bool uploadOne(VkUploadContext::Recorder recorder, VkBuffer materialBuffer,
               VkDeviceSize dstOffsetBytes, const MaterialGPU &material,
               VkPipelineStageFlags dstStage) {
  if (!recorder || materialBuffer == VK_NULL_HANDLE) {
    return false;
  }

  constexpr VkDeviceSize bytes = sizeof(MaterialGPU);

  VkStagingAlloc stage = recorder.allocStaging(bytes, /*alignment=*/16);
  if (!stage) {
    LOGE("allocStaging failed");
    return false;
  }

  std::memcpy(stage.ptr, &material, sizeof(MaterialGPU));

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadMemcpyCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::UploadMemcpyBytes, bytes);

  recorder.cmdCopyToBuffer(materialBuffer, dstOffsetBytes, stage.offset, bytes);
  recorder.cmdBarrierBufferTransferToShader(materialBuffer, dstOffsetBytes,
                                            bytes, dstStage);

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::MaterialUploadCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::MaterialUploadBytes,
                     static_cast<uint64_t>(bytes));

  return true;
}

} // namespace MaterialUploader
