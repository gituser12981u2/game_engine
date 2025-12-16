#include "vk_render_pass.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkRenderPassObj::init(VkDevice device, VkFormat swapchainColorFormat) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[RenderPass] Device is null\n";
    return false;
  }

  if (swapchainColorFormat == VK_FORMAT_UNDEFINED) {
    std::cerr << "[RenderPass] Swapchain image format undefined\n";
    return false;
  }

  // If re-init
  shutdown();
  m_device = device;

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainColorFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult res =
      vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
  if (res != VK_SUCCESS) {
    std::cerr << "[RenderPass] vkCreateRenderPass() failed: " << res << "\n";
    m_renderPass = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[RenderPass] Render pass created\n";
  return true;
}

void VkRenderPassObj::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE && m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  }
  m_renderPass = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}
