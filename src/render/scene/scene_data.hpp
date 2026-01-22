#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/buffers/vk_buffer.hpp"
#include "backend/gpu/buffers/vk_per_frame_uniform_buffers.hpp"
#include "backend/gpu/descriptors/vk_scene_sets.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/gpu/upload/vk_instance_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "engine/camera/camera_ubo.hpp"
#include "render/scene/debug_ubo.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <span>
#include <vulkan/vulkan_core.h>

class SceneData {
public:
  SceneData() = default;
  ~SceneData() noexcept { shutdown(); }

  SceneData(const SceneData &) = delete;
  SceneData &operator=(const SceneData &) = delete;

  SceneData(SceneData &&) noexcept = default;
  SceneData &operator=(SceneData &&) noexcept = default;

  bool init(VkBackendCtx &ctx, uint32_t framesInFlight,
            const VkShaderInterface &interface,
            uint32_t requestedMaxInstancesPerFrame,
            uint32_t requestedMaxMaterials);
  void shutdown() noexcept;

  bool update(uint32_t frameIndex, const CameraUBO &camera);
  void bind(VkCommandBuffer cmd, const VkShaderInterface &interface,
            uint32_t frameIndex) const;

  InstanceUploadResult uploadInstances(VkUploadContext::Recorder recorder,
                                       uint32_t frameIndex,
                                       uint32_t &cursorInstances,
                                       std::span<const glm::mat4> models);

  [[nodiscard]] VkBuffer materialBuffer() const noexcept {
    return m_materialBuf.handle();
  }
  [[nodiscard]] uint32_t materialCapacity() const noexcept {
    return m_materialCapacity;
  }
  [[nodiscard]] VkDeviceSize materialTableBytes() const noexcept {
    return m_materialTableBytes;
  }

  [[nodiscard]] VkBuffer instanceBuffer() const noexcept {
    return m_instanceBuf.handle();
  }
  [[nodiscard]] VkDeviceSize instanceFrameStride() const noexcept {
    return m_instanceFrameStride;
  }
  [[nodiscard]] uint32_t maxInstancesPerFrame() const noexcept {
    return m_maxInstancesPerFrame;
  }

  void setDebug(const DebugUBO &debug) { m_debug = debug; }

private:
  bool initCameraBuffers(VmaAllocator allocator, uint32_t framesInFlight);
  bool queryDeviceLimits(VkPhysicalDevice physicalDevice);
  bool initInstanceBuffer(VmaAllocator allocator, uint32_t framesInFlight,
                          uint32_t requestedMaxInstancesPerFrame);
  bool initMaterialBuffer(VmaAllocator allocator,
                          uint32_t requestedMaxMaterials);
  bool initDebugBuffers(VmaAllocator allocator, uint32_t framesInFlight);
  bool initDescriptorSets(VkDevice device, const VkShaderInterface &interface);

  VkPerFrameUniformBuffers m_cameraBufs; // sizeof(CameraUBO) per frame

  VkDeviceSize m_maxStorageBufferRange = 0;

  VkBufferObj m_materialBuf; // device-local storage buffer (global)
  uint32_t m_materialCapacity = 0;
  VkDeviceSize m_materialTableBytes = 0;

  VkBufferObj m_instanceBuf; // device-local storage buffer
  VkDeviceSize m_instanceFrameStride = 0;
  uint32_t m_maxInstancesPerFrame = 0;
  VkInstanceUploader m_instanceUploader;

  VkPerFrameUniformBuffers m_debugBufs;
  DebugUBO m_debug = {};

  VkSceneSets m_sets; // set 0 bindings
  bool m_initiailized = false;
};
