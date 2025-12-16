#include "vk_pipeline.hpp"
#include "../shaders/vulkan_shader.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vulkan/vulkan_core.h>

// TODO AFTER REFACTOR: make viewport/scissor dynamic
bool VkGraphicsPipeline::init(VkDevice device, VkRenderPass renderPass,
                              VkExtent2D extent, const std::string &vertSpvPath,
                              const std::string &fragSpvPath) {
  if (device == VK_NULL_HANDLE || renderPass == VK_NULL_HANDLE) {
    std::cerr << "[Pipeline] Device or render pass not initialized\n";
    return false;
  }

  if (extent.width == 0 || extent.height == 0) {
    std::cerr << "[Pipeline] Extent is 0 height and 0 width\n";
    return false;
  }

  // Re-init
  shutdown();
  m_device = device;

  VulkanShaderModule vertModule;
  VulkanShaderModule fragModule;

  if (!createShaderModuleFromFile(m_device, vertSpvPath, vertModule)) {
    std::cerr << "[Pipeline] Failed to load vertex shader\n";
    return false;
  }

  if (!createShaderModuleFromFile(m_device, fragSpvPath, fragModule)) {
    std::cerr << "[Pipeline] Failed to load fragment shader\n";
    return false;
  }

  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = vertModule.m_handle;
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = fragModule.m_handle;
  fragStage.pName = "main";

  if (!createPipelineLayout()) {
    shutdown();
    return false;
  }

  const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{vertStage,
                                                                    fragStage};
  if (!createGraphicsPipeline(renderPass, extent, shaderStages.data(),
                              static_cast<uint32_t>(shaderStages.size()))) {
    shutdown();
    return false;
  }

  std::cout << "[Pipeline] Graphics pipeline created\n";
  return true;
}

bool VkGraphicsPipeline::createPipelineLayout() {
  // Pipeline layout: no descriptors/push constants for now
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  const VkResult res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo,
                                              nullptr, &m_pipelineLayout);
  if (res != VK_SUCCESS) {
    std::cerr << "[Pipeline] vkCreatePipelineLayout failed: " << res << "\n";
    m_pipelineLayout = VK_NULL_HANDLE;
    return false;
  }

  return true;
}

bool VkGraphicsPipeline::createGraphicsPipeline(
    VkRenderPass renderPass, VkExtent2D extent,
    const VkPipelineShaderStageCreateInfo *stages, uint32_t stageCount) {
  // TODO: Vertex input: none for now
  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 0;
  vertexInput.pVertexBindingDescriptions = nullptr;
  vertexInput.vertexAttributeDescriptionCount = 0;
  vertexInput.pVertexAttributeDescriptions = nullptr;

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport / scissor
  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;

  VkRect2D scissor{};
  scissor.offset = VkOffset2D{0, 0};
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0F;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  // Multisampling: off for now
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.sampleShadingEnable = VK_FALSE;

  // Color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  // Graphics pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = stageCount;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // TODO: add
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  const VkResult res = vkCreateGraphicsPipelines(
      m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
  if (res != VK_SUCCESS) {
    std::cerr << "[Pipeline] vkCreateGraphicsPipelines() failed: " << res
              << "\n";
    m_graphicsPipeline = VK_NULL_HANDLE;
    return false;
  }

  return true;
}

void VkGraphicsPipeline::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }
  }

  m_graphicsPipeline = VK_NULL_HANDLE;
  m_pipelineLayout = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}
