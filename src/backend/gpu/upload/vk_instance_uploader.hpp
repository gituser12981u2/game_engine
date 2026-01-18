#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"

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
  bool init();
  void shutdown() noexcept;

  // TODO: make cursorInstances multi threaded for parallelized
  // batching/instance writes
  InstanceUploadResult uploadMat4Instances(VkUploadContext::Recorder recorder,
                                           VkBuffer instanceBuffer,
                                           VkDeviceSize frameBaseBytes,
                                           VkDeviceSize frameStrideBytes,
                                           uint32_t maxInstancesPerFrame,
                                           uint32_t &cursorInstances,
                                           std::span<const glm::mat4> models);
};
