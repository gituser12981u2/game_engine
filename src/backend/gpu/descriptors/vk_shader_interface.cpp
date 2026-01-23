#include "backend/gpu/descriptors/vk_shader_interface.hpp"

#include "engine/logging/log.hpp"
#include "render/scene/push_constants.hpp"

#include <array>
#include <cstdint>
#include <fmt/format.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>

bool VkShaderInterface::init(VkDevice device, uint32_t framesInFlight,
                             uint32_t maxTextures) {
  if (device == VK_NULL_HANDLE) {
    LOGE("Initialization invalid arguments");
    return false;
  }

  shutdown();

  m_device = device;
  m_maxTextures = maxTextures;

  // set=0 binding=1: scene per frame bindless array
  VkDescriptorSetLayoutBinding sceneUbo{};
  sceneUbo.binding = 0;
  sceneUbo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sceneUbo.descriptorCount = framesInFlight;
  sceneUbo.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  // set=0 binding=1: instance SSBO
  VkDescriptorSetLayoutBinding instanceSsbo{};
  instanceSsbo.binding = 1;
  instanceSsbo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  instanceSsbo.descriptorCount = 1;
  instanceSsbo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // set=0 binding=2: material table SSBO
  VkDescriptorSetLayoutBinding materialSsbo{};
  materialSsbo.binding = 2;
  materialSsbo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  materialSsbo.descriptorCount = 1;
  materialSsbo.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 3> sceneBindings{
      sceneUbo, instanceSsbo, materialSsbo};

  std::array<VkDescriptorBindingFlags, 3> sceneBindFlags{};
  sceneBindFlags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
  sceneBindFlags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

  VkDescriptorSetLayoutBindingFlagsCreateInfo sceneFlagsInfo{};
  sceneFlagsInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  sceneFlagsInfo.bindingCount = (uint32_t)sceneBindFlags.size();
  sceneFlagsInfo.pBindingFlags = sceneBindFlags.data();

  VkDescriptorSetLayoutCreateInfo sceneInfo{};
  sceneInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  sceneInfo.pNext = &sceneFlagsInfo;
  sceneInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
  sceneInfo.bindingCount = (uint32_t)(sceneBindings.size());
  sceneInfo.pBindings = sceneBindings.data();

  VkResult res = vkCreateDescriptorSetLayout(m_device, &sceneInfo, nullptr,
                                             &m_setLayoutScene);
  if (res != VK_SUCCESS) {
    LOGE("Scene set layout creation failed: {}", fmt::underlying(res));
    shutdown();
    return false;
  }

  // set=1 binding=0: bindless sampler2D
  VkDescriptorSetLayoutBinding tex{};
  tex.binding = 0;
  tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  tex.descriptorCount = m_maxTextures;
  tex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // Binding flags for descriptor indexing
  VkDescriptorBindingFlags bindingFlags =
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

  VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
  flagsInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  flagsInfo.bindingCount = 1;
  flagsInfo.pBindingFlags = &bindingFlags;

  VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
  materialLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  materialLayoutInfo.pNext = &flagsInfo;
  materialLayoutInfo.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
  materialLayoutInfo.bindingCount = 1;
  materialLayoutInfo.pBindings = &tex;

  res = vkCreateDescriptorSetLayout(m_device, &materialLayoutInfo, nullptr,
                                    &m_setLayoutMaterial);
  if (res != VK_SUCCESS) {
    LOGE("Material set layout creation failed");
    shutdown();
    return false;
  }

  // Push constant (model matrix)
  VkPushConstantRange pushRange{};
  pushRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushRange.offset = 0;
  pushRange.size = sizeof(DrawPushConstants);

  std::array<VkDescriptorSetLayout, 2> setLayouts = {m_setLayoutScene,
                                                     m_setLayoutMaterial};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
  pipelineLayoutInfo.pSetLayouts = setLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushRange;

  res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr,
                               &m_pipelineLayout);
  if (res != VK_SUCCESS) {
    LOGE("vkCreatePipelineLayout failed: {}", fmt::underlying(res));
    m_pipelineLayout = VK_NULL_HANDLE;
    return false;
  }

  return true;
}

void VkShaderInterface::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }

    if (m_setLayoutMaterial != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(m_device, m_setLayoutMaterial, nullptr);
    }

    if (m_setLayoutScene != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(m_device, m_setLayoutScene, nullptr);
    }
  }

  m_maxTextures = 0;
  m_pipelineLayout = VK_NULL_HANDLE;
  m_setLayoutMaterial = VK_NULL_HANDLE;
  m_setLayoutScene = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}
