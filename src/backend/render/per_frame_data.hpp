#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/resources/buffers/vk_buffer.hpp"
#include "backend/resources/buffers/vk_per_frame_uniform_buffers.hpp"
#include "backend/resources/descriptors/vk_per_frame_sets.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"
#include "engine/camera/camera_ubo.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

// TODO add SSBO materials
class PerFrameData {
public:
  PerFrameData() = default;
  ~PerFrameData() noexcept { shutdown(); }

  PerFrameData(const PerFrameData &) = delete;
  PerFrameData &operator=(const PerFrameData &) = delete;

  PerFrameData(PerFrameData &&) noexcept = default;
  PerFrameData &operator=(PerFrameData &&) noexcept = default;

  bool init(VkBackendCtx &ctx, uint32_t framesInFlight,
            const VkShaderInterface &interface);
  void shutdown() noexcept;

  bool update(uint32_t frameIndex, const CameraUBO &camera);
  void bind(VkCommandBuffer cmd, const VkShaderInterface &interface,
            uint32_t frameIndex) const;

  [[nodiscard]] VkBuffer instanceBuffer() const noexcept {
    return m_instanceBuf.handle();
  }
  [[nodiscard]] VkDeviceSize instanceFrameStride() const noexcept {
    return m_instanceFrameStride;
  }
  [[nodiscard]] uint32_t maxInstancesPerFrame() const noexcept {
    return m_maxInstancesPerFrame;
  }

private:
  VkPerFrameUniformBuffers m_cameraBufs; // sizeof(CameraUBO) per frame

  VkBufferObj m_instanceBuf; // device-local storage buffer
  VkDeviceSize m_instanceFrameStride = 0;
  uint32_t m_maxInstancesPerFrame = 0;

  VkPerFrameSets m_sets; // set 0 bindings
  bool m_initiailized = false;
};
