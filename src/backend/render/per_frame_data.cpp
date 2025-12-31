#include "backend/render/per_frame_data.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"
#include "engine/camera/camera_ubo.hpp"

#include <cstdint>
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

  if (!m_sets.init(device, interface.setLayoutPerFrame(), m_cameraBufs)) {
    std::cerr << "[PerFarmeData] Failed to init per-frame descriptor sets\n";
    shutdown();
    return false;
  }

  m_initiailized = true;
  return true;
}

void PerFrameData::shutdown() noexcept {
  m_sets.shutdown();
  m_cameraBufs.shutdown();
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
