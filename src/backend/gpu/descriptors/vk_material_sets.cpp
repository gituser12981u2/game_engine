#include "backend/gpu/descriptors/vk_material_sets.hpp"

#include "backend/gpu/textures/vk_texture.hpp"
#include "engine/logging/log.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <vulkan/vulkan_core.h>

bool VkMaterialSets::init(VkDevice device, VkDescriptorSetLayout layout,
                          uint32_t maxTexSrgb, uint32_t maxTexLin) {
  if (layout == VK_NULL_HANDLE || maxTexSrgb == 0 || maxTexLin == 0) {
    LOGE("Invalid initlization args");
    return false;
  }

  shutdown();

  m_device = device;
  m_layout = layout;
  m_maxSrgb = maxTexSrgb;
  m_maxLinear = maxTexLin;

  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = maxTexSrgb + maxTexLin;

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
  m_maxSrgb = 0;
  m_maxLinear = 0;
}

static bool writeOne(VkDevice device, VkDescriptorSet set, uint32_t binding,
                     uint32_t arrayElement, const VkTexture2D &tex) {
  if (!tex.valid()) {
    return false;
  }

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = tex.view;
  imgInfo.sampler = tex.sampler;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = set;
  write.dstBinding = binding;
  write.dstArrayElement = arrayElement;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  return true;
}

bool VkMaterialSets::writeSrgb(uint32_t index, const VkTexture2D &tex) {
  if (m_set == VK_NULL_HANDLE) {
    return false;
  }

  if (index >= m_maxSrgb) {
    return false;
  }

  return writeOne(m_device, m_set, /*binding=*/0, index, tex);
}

bool VkMaterialSets::writeLinear(uint32_t index, const VkTexture2D &tex) {
  if (m_set == VK_NULL_HANDLE) {
    return false;
  }

  if (index >= m_maxLinear) {
    return false;
  }

  return writeOne(m_device, m_set, /*binding=*/1, index, tex);
}

void VkMaterialSets::bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                          uint32_t setIndex) const {
  if (m_set == VK_NULL_HANDLE) {
    return;
  }

  // TODO: upgrade to dynamic offset
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &m_set, 0, nullptr);
}
