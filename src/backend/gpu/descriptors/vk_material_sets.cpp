#include "backend/gpu/descriptors/vk_material_sets.hpp"

#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <vulkan/vulkan_core.h>

bool VkMaterialSets::init(VkDevice device, VkDescriptorSetLayout layout,
                          uint32_t maxTextures) {
  if (layout == VK_NULL_HANDLE || maxTextures == 0) {
    LOGE("Invalid initlization args");
    return false;
  }

  shutdown();

  m_device = device;
  m_layout = layout;
  m_maxTextures = maxTextures;

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = maxTextures;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;

  VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    LOGE("vkCreateDescriptorPool failed: {}", fmt::underlying(res));
    shutdown();
    return false;
  }

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &m_layout;

  res = vkAllocateDescriptorSets(m_device, &allocInfo, &m_set);
  if (res != VK_SUCCESS) {
    LOGE("vkAllocateDescriptorSets failed: {}", fmt::underlying(res));
    shutdown();
    return false;
  }

  return true;
}

void VkMaterialSets::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    }
  }

  m_pool = VK_NULL_HANDLE;
  m_layout = VK_NULL_HANDLE;
  m_set = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
  m_maxTextures = 0;
}

bool VkMaterialSets::writeTexture(uint32_t slot, const VkTexture2D &texture) {
  if (!texture.valid() || slot >= m_maxTextures || m_set == VK_NULL_HANDLE) {
    return false;
  }

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = texture.view;
  imgInfo.sampler = texture.sampler;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = m_set;
  write.dstBinding = 0;
  write.dstArrayElement = slot;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  return true;
}

void VkMaterialSets::bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                          uint32_t setIndex) const {
  if (m_set == VK_NULL_HANDLE) {
    return;
  }

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &m_set, 0, nullptr);
  PROFILE_CPU_INC_DESCRIPTOR_BINDS(1);
}
