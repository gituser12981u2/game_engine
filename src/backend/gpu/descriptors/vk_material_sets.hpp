#pragma once

#include "backend/gpu/textures/vk_texture.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkMaterialSets {
public:
  VkMaterialSets() = default;
  ~VkMaterialSets() noexcept { shutdown(); }

  VkMaterialSets(const VkMaterialSets &) = delete;
  VkMaterialSets &operator=(const VkMaterialSets &) = delete;

  VkMaterialSets(VkMaterialSets &&other) noexcept { *this = std::move(other); }
  VkMaterialSets &operator=(VkMaterialSets &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pool = std::exchange(other.m_pool, VK_NULL_HANDLE);
    m_layout = std::exchange(other.m_layout, VK_NULL_HANDLE);
    m_set = std::move(other.m_set);

    return *this;
  }

  bool init(VkDevice device, VkDescriptorSetLayout layout, uint32_t maxTexSrgb,
            uint32_t maxTexLin);
  void shutdown() noexcept;

  bool writeSrgb(uint32_t index, const VkTexture2D &tex);
  bool writeLinear(uint32_t index, const VkTexture2D &tex);

  void bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            uint32_t setIndex) const;

  [[nodiscard]] VkDescriptorSet set() const noexcept { return m_set; }
  [[nodiscard]] VkDescriptorSetLayout layout() const noexcept {
    return m_layout;
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;              // non-owning
  VkDescriptorPool m_pool = VK_NULL_HANDLE;        // non-owning
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE; // non-owning
  VkDescriptorSet m_set = VK_NULL_HANDLE;
  uint32_t m_maxSrgb = 0;
  uint32_t m_maxLinear = 0;
};
