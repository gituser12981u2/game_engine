#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "render/resources/material_gpu.hpp"

#include <vulkan/vulkan_core.h>

class UploadProfiler;

class VkMaterialUploader {
public:
  bool init(UploadProfiler *profiler) {
    m_profiler = profiler;
    return true;
  }

  void shutdown() noexcept { m_profiler = nullptr; }

  bool uploadOne(
      VkUploadContext::Recorder recorder, VkBuffer materialBuffer,
      VkDeviceSize dstOffsetBytes, const MaterialGPU &material,
      VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

private:
  UploadProfiler *m_profiler = nullptr; // non-owning
};
