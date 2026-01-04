#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <span>
#include <vulkan/vulkan_core.h>

struct InstanceUploadResult {
  uint32_t baseInstance = 0;
  uint32_t instanceCount = 0;
  explicit operator bool() const noexcept { return instanceCount != 0; }
};

class VkInstanceUploader {
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

  InstanceUploadResult uploadMat4Instances(VkBuffer instanceBuffer,
                                           VkDeviceSize frameBaseBytes,
                                           VkDeviceSize frameStrideBytes,
                                           uint32_t maxInstancesPerFrame,
                                           uint32_t &cursorInstances,
                                           std::span<const glm::mat4> models);

private:
  VkUploadContext *m_upload = nullptr;  // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning
};
