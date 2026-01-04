#include "backend/resources/upload/vk_material_uploader.hpp"

#include "backend/profiling/upload_profiler.hpp"
#include "backend/render/resources/material_gpu.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"

#include <cstring>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkMaterialUploader::uploadOne(VkBuffer materialBuffer,
                                   VkDeviceSize dstOffsetBytes,
                                   const MaterialGPU &material) {
  if (m_upload == nullptr || materialBuffer == VK_NULL_HANDLE) {
    return false;
  }

  constexpr VkDeviceSize bytes = sizeof(MaterialGPU);

  VkStagingAlloc stage = m_upload->allocStaging(bytes, /*alignment=*/16);
  if (!stage) {
    std::cerr << "[MaterialUploader] allocStaging failed\n";
    return false;
  }

  std::memcpy(stage.ptr, &material, sizeof(MaterialGPU));

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, bytes);
  }

  m_upload->cmdCopyToBuffer(materialBuffer, dstOffsetBytes, stage.offset,
                            bytes);
  m_upload->cmdBarrierBufferTransferToFragmentShader(materialBuffer,
                                                     dstOffsetBytes, bytes);

  return true;
}
