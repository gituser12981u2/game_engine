#include "render/scene/scene_data.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "engine/camera/camera_ubo.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

bool SceneData::init(VkBackendCtx &ctx, uint32_t framesInFlight,
                     const VkShaderInterface &interface,
                     uint32_t requestedMaxInstancesPerFrame,
                     uint32_t requestedMaxMaterials, UploadProfiler *profiler) {
  shutdown();

  m_profiler = profiler;

  if (framesInFlight == 0) {
    std::cerr << "[SceneData] framesInFlight must be greater than 0\n";
    return false;
  }

  if (requestedMaxInstancesPerFrame == 0) {
    std::cerr << "[SceneData] requestedMaxInstancesPerFrame must be > 0\n";
    return false;
  }

  if (requestedMaxMaterials == 0) {
    std::cerr << "[SceneData] requestedMaxMaterials must be > 0\n";
    return false;
  }

  if (!queryDeviceLimits(ctx.physicalDevice())) {
    shutdown();
    return false;
  }

  if (!initCameraBuffers(ctx.allocator(), framesInFlight)) {
    shutdown();
    return false;
  }

  if (!initInstanceBuffer(ctx.allocator(), framesInFlight,
                          requestedMaxInstancesPerFrame)) {
    shutdown();
    return false;
  }

  if (!initMaterialBuffer(ctx.allocator(), requestedMaxMaterials)) {
    shutdown();
    return false;
  }

  if (!initDescriptorSets(ctx.device(), interface)) {
    shutdown();
    return false;
  }

  m_initiailized = true;
  return true;
}

bool SceneData::queryDeviceLimits(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(physicalDevice, &props);

  m_maxStorageBufferRange = props.limits.maxStorageBufferRange;
  if (m_maxStorageBufferRange == 0) {
    std::cerr << "[SceneData] maxStorageBufferRange is 0\n";
    return false;
  }

  return true;
}

bool SceneData::initCameraBuffers(VmaAllocator allocator,
                                  uint32_t framesInFlight) {
  if (!m_cameraBufs.init(allocator, framesInFlight, sizeof(CameraUBO))) {
    std::cerr << "[SceneData] Failed to init camera UBO buffers\n";
    return false;
  }

  return true;
}

bool SceneData::initInstanceBuffer(VmaAllocator allocator,
                                   uint32_t framesInFlight,
                                   uint32_t requestedMaxInstancesPerFrame) {
  m_maxInstancesPerFrame = requestedMaxInstancesPerFrame;

  VkDeviceSize wantedStride =
      VkDeviceSize(m_maxInstancesPerFrame) * sizeof(glm::mat4);

  // Clamp to maxStorageBufferRange
  if (wantedStride > m_maxStorageBufferRange) {
    m_maxInstancesPerFrame =
        static_cast<uint32_t>(m_maxStorageBufferRange / sizeof(glm::mat4));
    wantedStride = VkDeviceSize(m_maxInstancesPerFrame) * sizeof(glm::mat4);
  }

  if (m_maxInstancesPerFrame == 0 || wantedStride == 0) {
    std::cerr << "[SceneData] maxStorageBufferRange too small for instances\n";
    return false;
  }

  m_instanceFrameStride = wantedStride;

  const VkDeviceSize totalBytes =
      VkDeviceSize(framesInFlight) * m_instanceFrameStride;

  const VkBufferUsageFlags usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_instanceBuf.init(allocator, totalBytes, usage,
                          VkBufferObj::MemUsage::GpuOnly, /*mapped*/ false)) {
    std::cerr << "[SceneData] Failed to create instance SSBO\n";
    return false;
  }

  if (m_profiler != nullptr) {
    profilerAdd(m_profiler, UploadProfiler::Stat::InstanceAllocatedBytes,
                static_cast<std::uint64_t>(totalBytes));
  }

  return true;
}

bool SceneData::initMaterialBuffer(VmaAllocator allocator,
                                   uint32_t requestedMaxMaterials) {
  m_materialCapacity = requestedMaxMaterials;
  m_materialTableBytes = VkDeviceSize(m_materialCapacity) * sizeof(MaterialGPU);

  // Clamp to maxStorageBufferRange
  if (m_materialTableBytes > m_maxStorageBufferRange) {
    m_materialCapacity =
        static_cast<uint32_t>(m_maxStorageBufferRange / sizeof(MaterialGPU));
    m_materialTableBytes =
        VkDeviceSize(m_materialCapacity) * sizeof(MaterialGPU);
  }

  if (m_materialCapacity == 0 || m_materialTableBytes == 0) {
    std::cerr << "[SceneData] maxStorageBufferRange too small for materials\n";
    return false;
  }

  const VkBufferUsageFlags matUsage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_materialBuf.init(allocator, m_materialTableBytes, matUsage,
                          VkBufferObj::MemUsage::GpuOnly, /*mapped*/ false)) {
    std::cerr << "[SceneData] Failed to create instance SSBO\n";
    return false;
  }

  return true;
}

bool SceneData::initDescriptorSets(VkDevice device,
                                   const VkShaderInterface &interface) {
  if (!m_sets.init(device, interface.setLayoutScene(), m_cameraBufs,
                   m_instanceBuf.handle(), m_instanceFrameStride,
                   m_materialBuf.handle(), m_materialTableBytes)) {
    std::cerr << "[SceneData] Failed to init scene descriptor sets\n";
    return false;
  }
  return true;
}

void SceneData::shutdown() noexcept {
  m_instanceUploader.shutdown();
  m_sets.shutdown();
  m_materialBuf.shutdown();
  m_instanceBuf.shutdown();
  m_cameraBufs.shutdown();

  m_instanceFrameStride = 0;
  m_maxInstancesPerFrame = 0;
  m_materialTableBytes = 0;

  m_profiler = nullptr;

  m_initiailized = false;
}

bool SceneData::update(uint32_t frameIndex, const CameraUBO &camera) {
  if (!m_initiailized) {
    return false;
  }

  if (!m_cameraBufs.update(frameIndex, &camera, sizeof(CameraUBO))) {
    std::cerr << "[PerFrameData] Failed to update camera UBO\n";
    return false;
  }

  return true;
}

void SceneData::bind(VkCommandBuffer cmd, const VkShaderInterface &interface,
                     uint32_t frameIndex) const {
  if (!m_initiailized) {
    return;
  }

  // set 0
  m_sets.bind(cmd, interface.pipelineLayout(), 0, frameIndex);
}

bool SceneData::rebindUpload(VkUploadContext &upload,
                             UploadProfiler *profiler) {
  return m_instanceUploader.init(&upload, profiler);
}

InstanceUploadResult
SceneData::uploadInstances(uint32_t frameIndex, uint32_t &cursorInstances,
                           std::span<const glm::mat4> models) {
  const VkDeviceSize frameBase =
      VkDeviceSize(frameIndex) * m_instanceFrameStride;

  return m_instanceUploader.uploadMat4Instances(
      m_instanceBuf.handle(), frameBase, m_instanceFrameStride,
      m_maxInstancesPerFrame, cursorInstances, models);
}
