#include "backend/render/per_frame_data.hpp"
#include "backend/resources/buffers/vk_buffer.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"
#include "engine/camera/camera_ubo.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool PerFrameData::init(VkBackendCtx &ctx, uint32_t framesInFlight,
                        const VkShaderInterface &interface) {
  shutdown();

  if (framesInFlight == 0) {
    std::cerr << "[PerFrameData] framesInFlight must be greater than 0\n";
  }

  VkDevice device = ctx.device();
  VmaAllocator allocator = ctx.allocator();

  if (!m_cameraBufs.init(allocator, framesInFlight, sizeof(CameraUBO))) {
    std::cerr << "[PerFrameData] Failed to init camera UBO buffers\n";
    shutdown();
    return false;
  }

  // TODO make a big global buffer that is not an SSBO per frame
  // to make this more GPU bound and add indirect draw calls
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(ctx.physicalDevice(), &props);
  const VkDeviceSize maxRange = props.limits.maxStorageBufferRange;

  // TODO: make input into per_frame_data
  m_maxInstancesPerFrame = 16 * 1024; // 16k

  VkDeviceSize wantedStride =
      VkDeviceSize(m_maxInstancesPerFrame) * sizeof(glm::mat4);

  // Clamp to maxStorageBufferRange
  if (wantedStride > maxRange) {
    m_maxInstancesPerFrame =
        static_cast<uint32_t>(maxRange / sizeof(glm::mat4));
    wantedStride = VkDeviceSize(m_maxInstancesPerFrame) * sizeof(glm::mat4);
  }

  if (m_maxInstancesPerFrame == 0 || wantedStride == 0) {
    std::cerr
        << "[PerFrameData] maxStorageBufferRange too small for instances\n";
    shutdown();
    return false;
  }

  m_instanceFrameStride = wantedStride;

  const VkDeviceSize totalBytes =
      VkDeviceSize(framesInFlight) * m_instanceFrameStride;

  const VkBufferUsageFlags usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (!m_instanceBuf.init(allocator, totalBytes, usage,
                          VkBufferObj::MemUsage::GpuOnly, /*mapped*/ false)) {
    std::cerr << "[PerFrameData] Failed to create instance SSBO\n";
    return false;
  }

  if (!m_sets.init(device, interface.setLayoutPerFrame(), m_cameraBufs,
                   m_instanceBuf.handle(), m_instanceFrameStride)) {
    std::cerr << "[PerFarmeData] Failed to init per-frame descriptor sets\n";
    shutdown();
    return false;
  }

  m_initiailized = true;
  return true;
}

void PerFrameData::shutdown() noexcept {
  m_sets.shutdown();
  m_instanceBuf.shutdown();
  m_cameraBufs.shutdown();

  m_instanceFrameStride = 0;
  m_maxInstancesPerFrame = 0;
  m_initiailized = false;
}

bool PerFrameData::update(uint32_t frameIndex, const CameraUBO &camera) {
  if (!m_initiailized) {
    return false;
  }

  if (!m_cameraBufs.update(frameIndex, &camera, sizeof(CameraUBO))) {
    std::cerr << "[PerFrameData] Failed to update camera UBO\n";
    return false;
  }

  return true;
}

void PerFrameData::bind(VkCommandBuffer cmd, const VkShaderInterface &interface,
                        uint32_t frameIndex) const {
  if (!m_initiailized) {
    return;
  }

  // set 0
  m_sets.bind(cmd, interface.pipelineLayout(), 0, frameIndex);
}
