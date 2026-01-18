#include "backend/gpu/upload/vk_instance_uploader.hpp"

#include "backend/profiling/telemetry/telemetry.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkInstanceUploader::init() { return true; }

void VkInstanceUploader::shutdown() noexcept {}

InstanceUploadResult VkInstanceUploader::uploadMat4Instances(
    VkUploadContext::Recorder recorder, VkBuffer instanceBuffer,
    VkDeviceSize frameBaseBytes, VkDeviceSize frameStrideBytes,
    uint32_t maxInstancesPerFrame, uint32_t &cursorInstances,
    std::span<const glm::mat4> models) {
  InstanceUploadResult out{};
  if (!recorder || instanceBuffer == VK_NULL_HANDLE) {
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

  VkStagingAlloc stageAlloc = recorder.allocStaging(bytes, /*alignment*/ 16);
  if (!stageAlloc) {
    std::cerr << "[InstanceUploader] allocStaging failed for instances\n";
    return out;
  }

  std::memcpy(stageAlloc.ptr, models.data(), static_cast<size_t>(bytes));

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::UploadMemcpyCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::UploadMemcpyBytes, bytes);

  const uint32_t base = cursorInstances;
  const VkDeviceSize dstOffset =
      frameBaseBytes + (VkDeviceSize(base) * sizeof(glm::mat4));

  recorder.cmdCopyToBuffer(instanceBuffer, dstOffset, stageAlloc.offset, bytes);
  recorder.cmdBarrierBufferTransferToShader(
      instanceBuffer, dstOffset, bytes, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

  cursorInstances += count;

  PROFILE_UPLOAD_INC(UploadProfiler::Stat::InstanceUploadCount);
  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::InstanceUploadBytes, bytes);

  out.baseInstance = base;
  out.instanceCount = count;

  return out;
}
