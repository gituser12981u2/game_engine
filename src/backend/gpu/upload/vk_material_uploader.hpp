#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "render/resources/material_gpu.hpp"

#include <vulkan/vulkan_core.h>

class VkMaterialUploader {
public:
  bool init(VkUploadContext *upload, UploadProfiler *profiler) {
    m_upload = upload;
    m_profiler = profiler;
    return m_upload != nullptr;
  }

  void shutdown() noexcept {
    m_upload = nullptr;
    m_profiler = nullptr;
  }

  bool uploadOne(VkBuffer materialBuffer, VkDeviceSize dstOffsetBytes,
                 const MaterialGPU &material);

private:
  VkUploadContext *m_upload = nullptr;  // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning
};
