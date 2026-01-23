#include "backend/gpu/descriptors/vk_scene_sets.hpp"

#include "backend/gpu/buffers/vk_per_frame_uniform_buffers.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"
#include "engine/logging/log.hpp"

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <vector>
#include <vulkan/vulkan_core.h>

bool VkSceneSets::init(VkDevice device, VkDescriptorSetLayout layout,
                       const VkPerFrameUniformBuffers &sceneBufs,
                       VkBuffer instanceBuffer,
                       VkDeviceSize instanceFrameStrideBytes,
                       VkBuffer materialBuffer,
                       VkDeviceSize materialTableBytes) {
  if (device == VK_NULL_HANDLE || layout == VK_NULL_HANDLE ||
      !sceneBufs.valid() || instanceBuffer == VK_NULL_HANDLE ||
      instanceFrameStrideBytes == 0 || materialBuffer == VK_NULL_HANDLE ||
      materialTableBytes == 0) {
    LOGE("Initialization arguments invalid");
    return false;
  }

  const uint32_t framesInFlight = sceneBufs.frameCount();
  if (framesInFlight == 0) {
    LOGE("sceneBufs has 0 frames");
    return false;
  }

  shutdown();

  m_device = device;
  m_framesInFlight = framesInFlight;

  // UBO descriptors: scene[frames] = frames
  // SSBO descriptors: instance[frames] + material[1] = frames + 1
  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = framesInFlight;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[1].descriptorCount = framesInFlight + 1;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();

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
  allocInfo.pSetLayouts = &layout;

  res = vkAllocateDescriptorSets(m_device, &allocInfo, &m_set);
  if (res != VK_SUCCESS) {
    LOGE("vkAllocateDescriptorSets failed: {}", fmt::underlying(res));
    shutdown();
    return false;
  }

  std::vector<VkDescriptorBufferInfo> sceneInfos(framesInFlight);
  std::vector<VkDescriptorBufferInfo> instanceInfos(framesInFlight);

  for (uint32_t i = 0; i < framesInFlight; ++i) {
    sceneInfos[i].buffer = sceneBufs.buffer(i).handle();
    sceneInfos[i].offset = 0;
    sceneInfos[i].range = sceneBufs.stride();

    instanceInfos[i].buffer = instanceBuffer;
    instanceInfos[i].offset = VkDeviceSize(i) * instanceFrameStrideBytes;
    instanceInfos[i].range = instanceFrameStrideBytes;
  }

  // Global material table
  VkDescriptorBufferInfo materialInfo{};
  materialInfo.buffer = materialBuffer;
  materialInfo.offset = 0;
  materialInfo.range = materialTableBytes;

  std::array<VkWriteDescriptorSet, 3> writes{};

  // binding 0: SceneUBO[]
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].dstSet = m_set;
  writes[0].dstBinding = 0;
  writes[0].dstArrayElement = 0;
  writes[0].descriptorCount = framesInFlight;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].pBufferInfo = sceneInfos.data();

  // binding 1: InstanceSSBO[]
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].dstSet = m_set;
  writes[1].dstBinding = 1;
  writes[1].dstArrayElement = 0;
  writes[1].descriptorCount = framesInFlight;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[1].pBufferInfo = instanceInfos.data();

  // binding 2: MaterialSSBO
  writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[2].dstSet = m_set;
  writes[2].dstBinding = 2;
  writes[2].dstArrayElement = 0;
  writes[2].descriptorCount = 1;
  writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writes[2].pBufferInfo = &materialInfo;

  vkUpdateDescriptorSets(m_device, (uint32_t)writes.size(), writes.data(), 0,
                         nullptr);

  return true;
}

void VkSceneSets::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
  }

  m_pool = VK_NULL_HANDLE;
  m_set = VK_NULL_HANDLE;
  m_framesInFlight = 0;
  m_device = VK_NULL_HANDLE;
}

void VkSceneSets::bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                       uint32_t setIndex) const {
  if (m_set == VK_NULL_HANDLE) {
    return;
  }

  // TODO: use dynamic offset to have on descriptor per object UBO ring buffer
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &m_set, 0, nullptr);
  PROFILE_CPU_INC_DESCRIPTOR_BINDS(1);
}
