#include "renderer.hpp"
#include "../presentation/vk_presenter.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool Renderer::init(VkDevice device, VkQueue graphicsQueue,
                    uint32_t graphicsQueueFamily, VkPresenter &presenter,
                    uint32_t framesInFlight, const std::string &vertSpvPath,
                    const std::string &fragSpvPath) {
  if (device == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
    std::cerr << "[Renderer] Invalid device/queue\n";
    return false;
  }

  if (graphicsQueueFamily == UINT32_MAX) {
    std::cerr << "[Renderer] Invalid graphics queue family\n";
    return false;
  }

  if (framesInFlight == 0) {
    std::cerr << "[Renderer] framesInFlight must be > 0\n";
    return false;
  }

  // Re-init
  shutdown();

  m_device = device;
  m_graphicsQueue = graphicsQueue;
  m_graphicsQueueFamily = graphicsQueueFamily;
  m_framesInFlight = framesInFlight;

  m_vertPath = vertSpvPath;
  m_fragPath = fragSpvPath;

  // Create render pass
  if (!m_renderPass.init(m_device, presenter.imageFormat())) {
    std::cerr << "[Renderer] Failed to create render pass\n";
    shutdown();
    return false;
  }

  // Create graphics pipeline
  if (!m_pipeline.init(m_device, m_renderPass.handle(), presenter.extent(),
                       m_vertPath, m_fragPath)) {
    std::cerr << "[Renderer] Failed to create graphics pipeline\n";
    shutdown();
    return false;
  }

  // Create frame buffers
  if (!m_framebuffers.init(m_device, m_renderPass.handle(),
                           presenter.imageViews(), presenter.extent())) {
    std::cerr << "[Renderer] Failed to create framebuffers\n";
    shutdown();
    return false;
  }

  if (!m_commands.init(m_device, m_graphicsQueueFamily)) {
    std::cerr << "[Renderer] Failed to create the command pool\n";
    shutdown();
    return false;
  }

  const uint32_t imageCount =
      static_cast<uint32_t>(presenter.imageViews().size());

  if (!m_commands.allocate(imageCount)) {
    std::cerr << "[Renderer] Failed to allocate command buffers\n";
    shutdown();
    return false;
  }

  if (!m_frames.init(m_device, m_framesInFlight, imageCount)) {
    std::cerr << "[Renderer] Failed to create frame sync objects\n";
    shutdown();
    return false;
  }
  return true;
}

// Destroy in reverse initialization order
void Renderer::shutdown() {
  // Frame manager
  m_frames.shutdown();
  m_commands.shutdown();

  // Swapchain-dependents
  m_framebuffers.shutdown();
  m_pipeline.shutdown();
  m_renderPass.shutdown();

  m_device = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_graphicsQueueFamily = UINT32_MAX;
  m_framesInFlight = 0;

  m_vertPath.clear();
  m_fragPath.clear();
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkFramebuffer fb,
                           VkExtent2D extent) {
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clear{};
  clear.color = {{0.05f, 0.05f, 0.08f, 1.0f}};

  VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBegin.renderPass = m_renderPass.handle();
  rpBegin.framebuffer = fb;
  rpBegin.renderArea.offset = {0, 0};
  rpBegin.renderArea.extent = extent;
  rpBegin.clearValueCount = 1;
  rpBegin.pClearValues = &clear;

  vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipeline.pipeline());
  vkCmdDraw(cmd, 3, 1, 0, 0);
  vkCmdEndRenderPass(cmd);

  vkEndCommandBuffer(cmd);
}

bool Renderer::drawFrame(VkPresenter &presenter) {
  if (m_device == VK_NULL_HANDLE) {
    return false;
  }

  uint32_t imageIndex = 0;
  if (!m_frames.beginFrame(presenter.swapchain(), imageIndex)) {
    return false;
  }

  VkCommandBuffer cmd = m_commands.buffers()[imageIndex];
  vkResetCommandBuffer(cmd, 0);

  recordFrame(cmd, m_framebuffers.at(imageIndex), presenter.extent());

  return m_frames.submitAndPresent(m_graphicsQueue, presenter.swapchain(),
                                   imageIndex, cmd);
}
