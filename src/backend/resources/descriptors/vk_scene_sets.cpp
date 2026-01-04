#include "backend/resources/descriptors/vk_scene_sets.hpp"

#include "backend/resources/buffers/vk_per_frame_uniform_buffers.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkSceneSets::init(VkDevice device, VkDescriptorSetLayout layout,
                       const VkPerFrameUniformBuffers &bufs,
                       VkBuffer instanceBuffer,
                       VkDeviceSize instanceFrameStrideBytes,
                       VkBuffer materialBuffer,
                       VkDeviceSize materialTableBytes) {
  if (device == VK_NULL_HANDLE || layout == VK_NULL_HANDLE || !bufs.valid() ||
      instanceBuffer == VK_NULL_HANDLE || instanceFrameStrideBytes == 0 ||
      materialBuffer == VK_NULL_HANDLE || materialTableBytes == 0) {
    std::cerr << "[PerFrameSets] init invalid args\n";
    return false;
  }

  shutdown();

  m_device = device;

  const uint32_t framesInFlight = bufs.frameCount();

  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = framesInFlight;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = framesInFlight * 2; // instance + materials

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = framesInFlight;
  poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();

  VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    std::cerr << "[PerFrameSets] vkCreateDescriptorPool failed: " << res
              << "\n";
    shutdown();
    return false;
  }

  std::vector<VkDescriptorSetLayout> layouts(framesInFlight, layout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_pool;
  allocInfo.descriptorSetCount = framesInFlight;
  allocInfo.pSetLayouts = layouts.data();

  m_sets.resize(framesInFlight);
  res = vkAllocateDescriptorSets(m_device, &allocInfo, m_sets.data());
  if (res != VK_SUCCESS) {
    std::cerr << "[PerFrameSets] Failed to allocate descriptor sets\n";
    shutdown();
    return false;
  }

  // Global material table
  VkDescriptorBufferInfo materialInfo{};
  materialInfo.buffer = materialBuffer;
  materialInfo.offset = 0;
  materialInfo.range = materialTableBytes;

  // Write set 0 bindings for each frame
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = bufs.buffer(i).handle();
    uboInfo.offset = 0;
    uboInfo.range = bufs.stride();

    VkDescriptorBufferInfo instanceInfo{};
    instanceInfo.buffer = instanceBuffer;
    instanceInfo.offset = VkDeviceSize(i) * instanceFrameStrideBytes;
    instanceInfo.range = instanceFrameStrideBytes;

    std::array<VkWriteDescriptorSet, 3> writes{};

    // binding 0: camera UBO
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_sets[i];
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &uboInfo;

    // binding 1: instance SSBO
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_sets[i];
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &instanceInfo;

    // binding 2: material table SSBO
    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = m_sets[i];
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &materialInfo;

    vkUpdateDescriptorSets(m_device, (uint32_t)writes.size(), writes.data(), 0,
                           nullptr);
  }

  return true;
}

void VkSceneSets::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
  }

  m_pool = VK_NULL_HANDLE;
  m_sets.clear();
  m_device = VK_NULL_HANDLE;
}

void VkSceneSets::bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                       uint32_t setIndex, uint32_t frameIndex) const {
  if (frameIndex >= m_sets.size()) {
    return;
  }

  VkDescriptorSet set = m_sets[frameIndex];

  // TODO: use dynamic offset to have on descriptor per object UBO ring buffer
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &set, 0, nullptr);
}
