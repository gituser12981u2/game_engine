#include "backend/gpu/upload/vk_material_uploader.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkMaterialUploader::uploadOne(VkUploadContext::Recorder recorder,
                                   VkBuffer materialBuffer,
                                   VkDeviceSize dstOffsetBytes,
                                   const MaterialGPU &material,
                                   VkPipelineStageFlags dstStage) {
  if (!recorder || materialBuffer == VK_NULL_HANDLE) {
    return false;
  }

  constexpr VkDeviceSize bytes = sizeof(MaterialGPU);

  VkStagingAlloc stage = recorder.allocStaging(bytes, /*alignment=*/16);
  if (!stage) {
    std::cerr << "[MaterialUploader] allocStaging failed\n";
    return false;
  }

  std::memcpy(stage.ptr, &material, sizeof(MaterialGPU));

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, bytes);
  }

  recorder.cmdCopyToBuffer(materialBuffer, dstOffsetBytes, stage.offset, bytes);
  recorder.cmdBarrierBufferTransferToShader(materialBuffer, dstOffsetBytes,
                                            bytes, dstStage);

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::MaterialUploadCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::MaterialUploadBytes,
                static_cast<uint64_t>(bytes));
  }

  return true;
}
