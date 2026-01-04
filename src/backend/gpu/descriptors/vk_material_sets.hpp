#pragma once

#include "backend/gpu/textures/vk_texture.hpp"

#include <cstdint>
#include <utility>
#include <vector>
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
    m_sets = std::move(other.m_sets);

    return *this;
  }

  bool init(VkDevice device, VkDescriptorSetLayout layout,
            uint32_t maxMaterials);
  void shutdown() noexcept;

  // TODO: change name to allocateBaseColorMaterial and then add
  // allocatePbrMaterial
  uint32_t allocateForTexture(const VkTexture2D &tex);

  void bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            uint32_t setIndex, uint32_t materialIndex) const;

  [[nodiscard]] VkDescriptorSetLayout layout() const noexcept {
    return m_layout;
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;              // non-owning
  VkDescriptorPool m_pool = VK_NULL_HANDLE;        // non-owning
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE; // non-owning
  std::vector<VkDescriptorSet> m_sets;
};
