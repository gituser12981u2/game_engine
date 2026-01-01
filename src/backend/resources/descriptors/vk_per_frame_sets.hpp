#pragma once

#include "backend/resources/buffers/vk_per_frame_uniform_buffers.hpp"

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkPerFrameSets {
public:
  VkPerFrameSets() = default;
  ~VkPerFrameSets() noexcept { shutdown(); }

  VkPerFrameSets(const VkPerFrameSets &) = delete;
  VkPerFrameSets &operator=(const VkPerFrameSets &) = delete;

  VkPerFrameSets(VkPerFrameSets &&other) noexcept { *this = std::move(other); }
  VkPerFrameSets &operator=(VkPerFrameSets &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_sets = std::move(other.m_sets);

    return *this;
  }

  bool init(VkDevice device, VkDescriptorSetLayout layout,
            const VkPerFrameUniformBuffers &uboBufs, VkBuffer instanceBuffer,
            VkDeviceSize instanceFrameStrideBytes);
  void shutdown() noexcept;

  void bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            uint32_t setIndex, uint32_t frameIndex) const;

  [[nodiscard]] bool valid() const noexcept { return m_pool != VK_NULL_HANDLE; }
  [[nodiscard]] VkDescriptorSet set(uint32_t frameIndex) const noexcept {
    return m_sets[frameIndex];
  }
  [[nodiscard]] uint32_t setCount() const noexcept {
    return static_cast<uint32_t>(m_sets.size());
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;       // non-owning
  VkDescriptorPool m_pool = VK_NULL_HANDLE; // owning
  std::vector<VkDescriptorSet> m_sets;
};
