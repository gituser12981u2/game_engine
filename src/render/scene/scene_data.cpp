#include "render/scene/scene_data.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/gpu/upload/vk_instance_uploader.hpp"
#include "backend/gpu/upload/vk_lights_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/camera/camera_ubo.hpp"
#include "engine/logging/log.hpp"
#include "render/resources/material_gpu.hpp"
#include "render/scene/lights_gpu.hpp"
#include "render/scene/scene_ubo.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

bool SceneData::init(VkBackendCtx &ctx, uint32_t framesInFlight,
                     const VkShaderInterface &interface,
                     uint32_t requestedMaxInstancesPerFrame,
                     uint32_t requestedMaxMaterials,
                     uint32_t requestedMaxPointLights) {
  shutdown();

  if (framesInFlight == 0) {
    LOGE("framesInFlight must be greater than 0");
    return false;
  }

  if (requestedMaxInstancesPerFrame == 0) {
    LOGE("requestedMaxInstancesPerFrame must be > 0");
    return false;
  }

  if (requestedMaxMaterials == 0) {
    LOGE("requestedMaxMaterials must be > 0");
    return false;
  }

  if (requestedMaxPointLights == 0) {
    LOGE("requestedMaxPointLights must be > 0");
  }

  if (!queryDeviceLimits(ctx.physicalDevice())) {
    shutdown();
    return false;
  }

  if (!initSceneBuffers(ctx.allocator(), framesInFlight)) {
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

  if (!initPointLightBuffer(ctx.allocator(), framesInFlight,
                            requestedMaxPointLights)) {
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
    LOGE("maxStorageBufferRange is 0");
    return false;
  }

  return true;
}

bool SceneData::initSceneBuffers(VmaAllocator allocator,
                                 uint32_t framesInFlight) {
  if (!m_sceneBufs.init(allocator, framesInFlight, sizeof(SceneUBO))) {
    LOGE("Camera UBO buffers initialization failed");
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
    LOGE("maxStorageBufferRange too small for instances");
    return false;
  }

  m_instanceFrameStride = wantedStride;

  const VkDeviceSize totalBytes =
      VkDeviceSize(framesInFlight) * m_instanceFrameStride;

  const VkBufferUsageFlags usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_instanceBuf.init(allocator, totalBytes, usage,
                          VkBufferObj::MemUsage::GpuOnly, /*mapped*/ false)) {
    LOGE("Instance SSBO creation failed");
    return false;
  }

  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::InstanceAllocatedBytes,
                     static_cast<uint64_t>(totalBytes));

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
    LOGE("maxStorageBufferRange too small for materials");
    return false;
  }

  const VkBufferUsageFlags matUsage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_materialBuf.init(allocator, m_materialTableBytes, matUsage,
                          VkBufferObj::MemUsage::GpuOnly, /*mapped*/ false)) {
    LOGE("Material table creation failed");
    return false;
  }

  PROFILE_UPLOAD_ADD(UploadProfiler::Stat::MaterialAllocatedBytes,
                     static_cast<uint64_t>(m_materialTableBytes));

  return true;
}

bool SceneData::initPointLightBuffer(VmaAllocator allocator,
                                     uint32_t framesInFlight,
                                     uint32_t requestedMaxPointLights) {
  m_maxPointLightsPerFrame = requestedMaxPointLights;

  VkDeviceSize wantedStride =
      VkDeviceSize(m_maxPointLightsPerFrame) * sizeof(PointLightGPU);

  // Clamp to maxStorageBufferRange
  if (wantedStride > m_maxStorageBufferRange) {
    m_maxPointLightsPerFrame =
        static_cast<uint32_t>(m_maxStorageBufferRange / sizeof(PointLightGPU));
    wantedStride =
        VkDeviceSize(m_maxPointLightsPerFrame) * sizeof(PointLightGPU);
  }

  if (m_maxPointLightsPerFrame == 0 || wantedStride == 0) {
    LOGE("maxStorageBufferRange too small for point lights");
    return false;
  }

  m_pointLightFrameStride = wantedStride;

  const VkDeviceSize totalBytes =
      VkDeviceSize(framesInFlight) * m_pointLightFrameStride;

  const VkBufferUsageFlags usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_pointLightBuf.init(allocator, totalBytes, usage,
                            VkBufferObj::MemUsage::GpuOnly, /*mapped=*/false)) {
    LOGE("Point light SSBO creation failed");
    return false;
  }

  // PROFILE_UPLOAD_ADD(UploadProfiler::Stat::LightAllocation,
  //                    static_cast<uint64_t>(totalBytes));

  return true;
}

bool SceneData::initDescriptorSets(VkDevice device,
                                   const VkShaderInterface &interface) {
  if (!m_sets.init(device, interface.setLayoutScene(), m_sceneBufs,
                   m_instanceBuf.handle(), m_instanceFrameStride,
                   m_materialBuf.handle(), m_materialTableBytes,
                   m_pointLightBuf.handle(), m_pointLightFrameStride)) {
    LOGE("Scene descriptor initialization failed");
    return false;
  }
  return true;
}

void SceneData::shutdown() noexcept {
  m_sets.shutdown();

  m_pointLightBuf.shutdown();
  m_materialBuf.shutdown();
  m_instanceBuf.shutdown();
  m_sceneBufs.shutdown();

  m_pointLightFrameStride = 0;
  m_maxPointLightsPerFrame = 0;
  m_pointLightCountThisFrame = 0;

  m_instanceFrameStride = 0;
  m_maxInstancesPerFrame = 0;
  m_materialTableBytes = 0;

  m_materialTableBytes = 0;
  m_materialCapacity = 0;

  m_initiailized = false;
}

bool SceneData::update(uint32_t frameIndex, const CameraUBO &camera) {
  if (!m_initiailized) {
    return false;
  }

  SceneUBO scene{};
  scene.camera = camera;
  scene.debug = m_debug;

  scene.pointLightCount = m_pointLightCountThisFrame;
  scene.dirLightCount = m_dirLightCountThisFrame;
  scene.dirLights = m_dirLights;

  if (!m_sceneBufs.update(frameIndex, &scene, sizeof(SceneUBO))) {
    LOGE("Scene UBO update failed");
    return false;
  }

  m_pointLightCountThisFrame = 0;
  m_dirLightCountThisFrame = 0;
  m_pointLightsScratch.clear();

  return true;
}

void SceneData::bind(VkCommandBuffer cmd,
                     const VkShaderInterface &interface) const {
  if (!m_initiailized) {
    return;
  }

  // set 0
  m_sets.bind(cmd, interface.pipelineLayout(), 0);
}

InstanceUploadResult
SceneData::uploadInstances(VkUploadContext::Recorder recorder,
                           uint32_t frameIndex, uint32_t &cursorInstances,
                           std::span<const glm::mat4> models) {
  const VkDeviceSize frameBase =
      VkDeviceSize(frameIndex) * m_instanceFrameStride;

  return InstanceUploader::uploadMat4Instances(
      recorder, m_instanceBuf.handle(), frameBase, m_instanceFrameStride,
      m_maxInstancesPerFrame, cursorInstances, models);
}

bool SceneData::uploadPointLights(VkUploadContext::Recorder recorder,
                                  uint32_t frameIndex,
                                  std::span<const PointLightGPU> lights) {
  const VkDeviceSize frameBase =
      VkDeviceSize(frameIndex) * m_pointLightFrameStride;

  const auto res = LightsUploader::uploadPointLights(
      recorder, m_pointLightBuf.handle(), frameBase, m_pointLightFrameStride,
      m_maxPointLightsPerFrame, lights);

  m_pointLightCountThisFrame = res.lightCount;
  return true;
}

// TODO: move into its own file
void SceneData::clearLights() {
  m_dirLightCountThisFrame = 0;
  m_pointLightsScratch.clear();
  m_pointLightCountThisFrame = 0;
}

void SceneData::addDirectionalLight(const DirectionalLight &light) {
  if (m_dirLightCountThisFrame >= kMaxDirLights) {
    LOGE("Direcitonal lights count this frame is higher than kMaxDirLights");
    return;
  }

  m_dirLights[m_dirLightCountThisFrame++] = light;
}

void SceneData::addPointLight(const PointLightGPU &light) {
  m_pointLightsScratch.push_back(light);
}

bool SceneData::commitLights(VkUploadContext::Recorder recorder,
                             uint32_t frameIndex) {
  if (!m_initiailized) {
    return false;
  }

  (void)uploadPointLights(recorder, frameIndex, m_pointLightsScratch);

  return true;
}
