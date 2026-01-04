#pragma once

#include "backend/gpu/buffers/vk_buffer.hpp"

#include <cstdint>
#include <utility>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkPerFrameUniformBuffers {
public:
  VkPerFrameUniformBuffers() = default;
  ~VkPerFrameUniformBuffers() noexcept { shutdown(); }

  VkPerFrameUniformBuffers(const VkPerFrameUniformBuffers &) = delete;
  VkPerFrameUniformBuffers &
  operator=(const VkPerFrameUniformBuffers &) = delete;

  VkPerFrameUniformBuffers(VkPerFrameUniformBuffers &&other) noexcept {
    *this = std::move(other);
  }
  VkPerFrameUniformBuffers &
  operator=(VkPerFrameUniformBuffers &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_allocator = std::exchange(other.m_allocator, nullptr);
    m_bufs = std::move(other.m_bufs);
    m_stride = std::exchange(other.m_stride, 0);

    return *this;
  }

  bool init(VmaAllocator allocator, uint32_t framesInFlight,
            VkDeviceSize strideBytes);
  void shutdown() noexcept;

  bool update(uint32_t frameIndex, const void *data, VkDeviceSize size);

  [[nodiscard]] VkDeviceSize stride() const noexcept { return m_stride; }
  [[nodiscard]] uint32_t frameCount() const noexcept {
    return static_cast<uint32_t>(m_bufs.size());
  }
  [[nodiscard]] const VkBufferObj &buffer(uint32_t frameIndex) const noexcept {
    return m_bufs[frameIndex];
  }
  [[nodiscard]] bool valid() const noexcept {
    return !m_bufs.empty() && m_stride != 0;
  }

private:
  VmaAllocator m_allocator = nullptr; // non-owning
  std::vector<VkBufferObj> m_bufs;    // owning
  VkDeviceSize m_stride = 0;
};
