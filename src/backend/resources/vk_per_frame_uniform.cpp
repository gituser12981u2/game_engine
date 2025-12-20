#include "vk_per_frame_uniform.hpp"
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkPerFrameUniform::init(VkPhysicalDevice physicalDevice, VkDevice device,
                             VkDescriptorSetLayout layout,
                             uint32_t framesInFlight,
                             VkDeviceSize strideBytes) {
  if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE ||
      layout == VK_NULL_HANDLE || framesInFlight == 0 || strideBytes == 0) {
    std::cerr << "[PerFrameUBO] Invalid init args\n";
    return false;
  }

  shutdown();
  m_device = device;
  m_stride = strideBytes;

  // Buffers
  m_bufs.resize(framesInFlight);
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    if (!m_bufs[i].init(physicalDevice, m_device, m_stride,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      std::cerr << "[PerFrameUBO] Failed to create UBO\n";
      shutdown();
      return false;
    }
  }

  // Descriptor pool
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = framesInFlight;

  VkDescriptorPoolCreateInfo poolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = framesInFlight;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;

  VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    std::cerr << "[PerFrameUBO] vkCreateDescriptorPool failed: " << res << "\n";
    shutdown();
    return false;
  }

  // Descriptor sets
  std::vector<VkDescriptorSetLayout> layouts(framesInFlight, layout);

  VkDescriptorSetAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = m_pool;
  allocInfo.descriptorSetCount = framesInFlight;
  allocInfo.pSetLayouts = layouts.data();

  m_sets.resize(framesInFlight);
  res = vkAllocateDescriptorSets(m_device, &allocInfo, m_sets.data());
  if (res != VK_SUCCESS) {
    std::cerr << "[PerFrameUBO] Failed to allocate descriptor sets\n";
    shutdown();
    return false;
  }

  // Write descriptors
  for (uint32_t i = 0; i < framesInFlight; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_bufs[i].handle();
    bufferInfo.offset = 0;
    bufferInfo.range = m_stride;

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_sets[i];
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
  }

  return true;
}

void VkPerFrameUniform::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    for (auto &b : m_bufs) {
      b.shutdown();
    }

    m_bufs.clear();

    if (m_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    }
  }

  m_pool = VK_NULL_HANDLE;
  m_sets.clear();
  m_stride = 0;
  m_device = VK_NULL_HANDLE;
}

bool VkPerFrameUniform::update(uint32_t frameIndex, const void *data,
                               VkDeviceSize size) {
  if (frameIndex >= m_bufs.size() || data == nullptr || size > m_stride) {
    std::cerr << "[PerFrameUBO] update invalid args\n";
    return false;
  }

  return m_bufs[frameIndex].upload(data, size);
}

void VkPerFrameUniform::bind(VkCommandBuffer cmd,
                             VkPipelineLayout pipelineLayout, uint32_t setIndex,
                             uint32_t frameIndex) const {
  if (frameIndex >= m_sets.size()) {
    return;
  }

  VkDescriptorSet set = m_sets[frameIndex];

  // TODO: use dynamic offset to have on descriptor per object UBO ring buffer
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &set, 0, nullptr);
}
