#include "backend/gpu/descriptors/vk_material_sets.hpp"

#include "backend/gpu/textures/vk_texture.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkMaterialSets::init(VkDevice device, VkDescriptorSetLayout layout,
                          uint32_t maxMaterials) {
  if (device == VK_NULL_HANDLE || layout == VK_NULL_HANDLE ||
      maxMaterials == 0) {
    std::cerr << "[MaterialSets] Invalid init args\n";
    return false;
  }

  shutdown();

  m_device = device;
  m_layout = layout;

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = maxMaterials;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = maxMaterials;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;

  VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    std::cerr << "[MaterialSets] vkCreateDescriptorPool failed: " << res
              << "\n";
    shutdown();
    return false;
  }

  m_sets.reserve(maxMaterials);
  return true;
}

void VkMaterialSets::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    }
  }

  m_pool = VK_NULL_HANDLE;
  m_sets.clear();
  m_layout = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}

uint32_t VkMaterialSets::allocateForTexture(const VkTexture2D &tex) {
  if (m_device == VK_NULL_HANDLE || m_pool == VK_NULL_HANDLE ||
      m_layout == VK_NULL_HANDLE) {
    std::cerr << "[MaterialSets] Not initialized\n";
    return UINT32_MAX;
  }

  if (!tex.valid()) {
    std::cerr << "[MaterialSets] Invalid texture\n";
    return UINT32_MAX;
  }

  VkDescriptorSet set = VK_NULL_HANDLE;
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &m_layout;

  VkResult res = vkAllocateDescriptorSets(m_device, &allocInfo, &set);
  if (res != VK_SUCCESS) {
    std::cerr << "[MaterialSets] vkAllocateDescriptorSets failed: " << res
              << "\n";
    return UINT32_MAX;
  }

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = tex.view;
  imgInfo.sampler = tex.sampler;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = set;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

  const uint32_t idx = static_cast<uint32_t>(m_sets.size());
  m_sets.push_back(set);

  return idx;
}

void VkMaterialSets::bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                          uint32_t setIndex, uint32_t materialIndex) const {
  if (materialIndex >= m_sets.size()) {
    return;
  }

  VkDescriptorSet set = m_sets[materialIndex];
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &set, 0, nullptr);
}
