#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <span>
#include <vulkan/vulkan_core.h>

class UploadProfiler;

struct InstanceUploadResult {
  uint32_t baseInstance = 0;
  uint32_t instanceCount = 0;
  explicit operator bool() const noexcept { return instanceCount != 0; }
};

class VkInstanceUploader {
public:
  bool init(UploadProfiler *profiler) {
    m_profiler = profiler;
    return true;
  }
  void shutdown() noexcept { m_profiler = nullptr; }

  // TODO: make cursorInstances multi threaded for parallelized
  // batching/instance writes
  InstanceUploadResult uploadMat4Instances(VkUploadContext::Recorder recorder,
                                           VkBuffer instanceBuffer,
                                           VkDeviceSize frameBaseBytes,
                                           VkDeviceSize frameStrideBytes,
                                           uint32_t maxInstancesPerFrame,
                                           uint32_t &cursorInstances,
                                           std::span<const glm::mat4> models);

private:
  UploadProfiler *m_profiler = nullptr; // non-owning
};
