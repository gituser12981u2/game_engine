#include "vk_shader_interface.hpp"

#include "backend/render/push_constants.hpp"

#include <array>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkShaderInterface::init(VkDevice device) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[ShaderInterface] init invalid args\n";
    return false;
  }

  shutdown();
  m_device = device;

  // set=0 binding=0: per frame UBO (camera)
  VkDescriptorSetLayoutBinding uboBinding{};
  uboBinding.binding = 0;
  uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboBinding.descriptorCount = 1;
  uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

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

  std::array<VkDescriptorSetLayoutBinding, 3> bindings{
      uboBinding, instanceBinding, materialBinding};

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

  // set=1 binding=0: Material sampler2D
  VkDescriptorSetLayoutBinding textureBinding{};
  textureBinding.binding = 0;
  textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureBinding.descriptorCount = 1;
  textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
  materialLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  materialLayoutInfo.bindingCount = 1;
  materialLayoutInfo.pBindings = &textureBinding;

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
  pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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

  m_pipelineLayout = VK_NULL_HANDLE;
  m_setLayoutMaterial = VK_NULL_HANDLE;
  m_setLayoutScene = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}
