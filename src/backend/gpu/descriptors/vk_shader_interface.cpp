#include "backend/gpu/descriptors/vk_shader_interface.hpp"

#include "render/scene/push_constants.hpp"

#include <array>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkShaderInterface::init(VkDevice device, uint32_t maxTextures) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[ShaderInterface] init invalid args\n";
    return false;
  }

  shutdown();

  m_device = device;
  m_maxTextures = maxTextures;

  // set=0 binding=0: camera UBO per frame
  VkDescriptorSetLayoutBinding cameraBinding{};
  cameraBinding.binding = 0;
  cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cameraBinding.descriptorCount = 1;
  cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // set=0 binding=1: instance SSBO
  VkDescriptorSetLayoutBinding instanceBinding{};
  instanceBinding.binding = 1;
  instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  instanceBinding.descriptorCount = 1;
  instanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // set=0 binding=2: material table SSBO
  VkDescriptorSetLayoutBinding materialBinding{};
  materialBinding.binding = 2;
  materialBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  materialBinding.descriptorCount = 1;
  materialBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  // set=0, binding=3: debug UBO
  VkDescriptorSetLayoutBinding debugBinding{};
  debugBinding.binding = 3;
  debugBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  debugBinding.descriptorCount = 1;
  debugBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 4> bindings{
      cameraBinding, debugBinding, instanceBinding, materialBinding};

  VkDescriptorSetLayoutCreateInfo perFrameInfo{};
  perFrameInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  perFrameInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  perFrameInfo.pBindings = bindings.data();

  VkResult res = vkCreateDescriptorSetLayout(m_device, &perFrameInfo, nullptr,
                                             &m_setLayoutScene);

  if (res != VK_SUCCESS) {
    std::cerr << "[ShaderInterface] create per-frame set layout failed: " << res
              << "\n";
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
    std::cerr << "[ShaderInterface] create material set layout failed: " << res
              << "\n";
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
    std::cerr << "[ShaderInterface] vkCreatePipelineLayout failed: " << res
              << "\n";
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
