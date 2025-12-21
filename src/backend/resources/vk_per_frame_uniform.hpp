#pragma once

#include "vk_buffer.hpp"
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkPerFrameUniform {
public:
  VkPerFrameUniform() = default;
  ~VkPerFrameUniform() noexcept { shutdown(); }

  VkPerFrameUniform(const VkPerFrameUniform &) = delete;
  VkPerFrameUniform &operator=(const VkPerFrameUniform &) = delete;

  VkPerFrameUniform(VkPerFrameUniform &&other) noexcept {
    *this = std::move(other);
  }
  VkPerFrameUniform &operator=(VkPerFrameUniform &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_sets = std::move(other.m_sets);
    m_bufs = std::move(other.m_bufs);
    m_stride = std::exchange(other.m_stride, 0);

    return *this;
  }

  bool init(VkPhysicalDevice physicalDevice, VkDevice device,
            VkDescriptorSetLayout layout, uint32_t framesInFlight,
            VkDeviceSize strideBytes);
  void shutdown() noexcept;

  bool update(uint32_t frameIndex, const void *data, VkDeviceSize size);

  void bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            uint32_t setIndex, uint32_t frameIndex) const;

  [[nodiscard]] bool valid() const noexcept { return m_pool != VK_NULL_HANDLE; }

private:
  VkDevice m_device = VK_NULL_HANDLE;       // non-owning
  VkDescriptorPool m_pool = VK_NULL_HANDLE; // owning

  std::vector<VkDescriptorSet> m_sets;
  std::vector<VkBufferObj> m_bufs;
  VkDeviceSize m_stride = 0;
};
