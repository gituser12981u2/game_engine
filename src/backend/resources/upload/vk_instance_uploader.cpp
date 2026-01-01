#include "backend/resources/upload/vk_instance_uploader.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

InstanceUploadResult VkInstanceUploader::uploadMat4Instances(
    VkBuffer instanceBuffer, VkDeviceSize frameBaseBytes,
    VkDeviceSize frameStrideBytes, uint32_t maxInstancesPerFrame,
    uint32_t &cursorInstances, std::span<const glm::mat4> models) {
  InstanceUploadResult out{};
  if (m_upload == nullptr || instanceBuffer == VK_NULL_HANDLE) {
    return out;
  }

  if (models.empty()) {
    return out;
  }

  const uint32_t count = static_cast<uint32_t>(models.size());
  if (cursorInstances + count > maxInstancesPerFrame) {
    // TODO: split batch
    std::cerr << "[InstanceUploader] Instance budget exceeded for frame\n";
    return out;
  }

  const VkDeviceSize bytes = VkDeviceSize(count) * sizeof(glm::mat4);

  const VkDeviceSize endBytes =
      VkDeviceSize(cursorInstances + count) * sizeof(glm::mat4);
  if (endBytes > frameStrideBytes) {
    std::cerr << "[InstanceUploader] exceeds SSBO descriptor range\n";
    return out;
  }

  VkStagingAlloc stageAlloc = m_upload->allocStaging(bytes, /*alignment*/ 16);
  if (!stageAlloc) {
    std::cerr << "[InstanceUploader] allocStaging failed for instances\n";
    return out;
  }

  std::memcpy(stageAlloc.ptr, models.data(), static_cast<size_t>(bytes));

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyCount, 1);
    profilerAdd(m_profiler, UploadProfiler::Stat::UploadMemcpyBytes, bytes);
  }

  const uint32_t base = cursorInstances;
  const VkDeviceSize dstOffset =
      frameBaseBytes + VkDeviceSize(base) * sizeof(glm::mat4);

  m_upload->cmdCopyToBuffer(instanceBuffer, dstOffset, stageAlloc.offset,
                            bytes);
  m_upload->cmdBarrierBufferTransferToVertexShader(instanceBuffer, dstOffset,
                                                   bytes);

  cursorInstances += count;

  out.baseInstance = base;
  out.instanceCount = count;

  return out;
}
