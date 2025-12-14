#include "backend/vulkan_context.hpp"
#include "vulkan_presenter.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

static void recordCommandBuffer(VkCommandBuffer cmd, VkRenderPass renderPass,
                                VkFramebuffer framebuffer, VkExtent2D extent,
                                VkPipeline pipeline) {

  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clear{};
  clear.color = {{0.05f, 0.05f, 0.08f, 1.0f}};

  VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBegin.renderPass = renderPass;
  rpBegin.framebuffer = framebuffer;
  rpBegin.renderArea.offset = {0, 0};
  rpBegin.renderArea.extent = extent;
  rpBegin.clearValueCount = 1;
  rpBegin.pClearValues = &clear;

  vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdDraw(cmd, 3, 1, 0, 0);
  vkCmdEndRenderPass(cmd);

  vkEndCommandBuffer(cmd);
}

int main() {
  GLFWwindow *window = nullptr;

  VulkanContext ctx;
  VulkanPresenter presenter;

  constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailable{};
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences{};
  std::vector<VkSemaphore> renderFinished;
  std::vector<VkFence> imagesInFlight;

  uint32_t currentFrame = 0;

  auto cleanup = [&]() {
    if (ctx.device() != VK_NULL_HANDLE)
      vkDeviceWaitIdle(ctx.device());

    for (auto s : imageAvailable)
      if (s)
        vkDestroySemaphore(ctx.device(), s, nullptr);

    for (auto s : renderFinished)
      if (s)
        vkDestroySemaphore(ctx.device(), s, nullptr);

    for (auto f : inFlightFences)
      if (f)
        vkDestroyFence(ctx.device(), f, nullptr);

    ctx.destroyCommandPool();
    ctx.destroyFramebuffers();
    presenter.shutdown(ctx);
    ctx.shutdown();

    if (window)
      glfwDestroyWindow(window);

    glfwTerminate();
  };

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Get needed GLFW instance extensions
  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  if (!glfwExts || glfwExtCount == 0) {
    std::cerr << "glfwGetRequiredInstanceExtensions returned nothing\n";
    cleanup();
    return 1;
  }

  std::vector<const char *> platformExtensions(glfwExts,
                                               glfwExts + glfwExtCount);

  if (!ctx.init(platformExtensions)) {
    std::cerr << "Failed to initialize Vulkan\n";
    cleanup();
    return 1;
  }

  window = glfwCreateWindow(800, 600, "Hello Window", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create window\n";
    cleanup();
    return 1;
  }

  // Get size (in pixels) for swapchain extent
  // TODO: Make this a helper function in vulkan_swapchain
  int fbWidth = 0, fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  if (fbWidth == 0 || fbHeight == 0) {
    int winWidth = 0;
    int winHeight = 0;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    fbWidth = winWidth;
    fbHeight = winHeight;
  }

  if (!presenter.init(ctx, window, (uint32_t)fbWidth, (uint32_t)fbHeight)) {
    std::cerr << "Failed to initialize presenter\n";
    cleanup();
    return 1;
  }

  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkCreateSemaphore(ctx.device(), &semInfo, nullptr, &imageAvailable[i]);
    vkCreateFence(ctx.device(), &fenceInfo, nullptr, &inFlightFences[i]);
  }

  const uint32_t imageCount = (uint32_t)presenter.imageViews().size();
  renderFinished.resize(imageCount);
  imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

  for (uint32_t i = 0; i < imageCount; ++i)
    vkCreateSemaphore(ctx.device(), &semInfo, nullptr, &renderFinished[i]);

  // Create render pass
  if (!ctx.createRenderPass(presenter.imageFormat())) {
    std::cerr << "Failed to create render pass\n";
    cleanup();
    return 1;
  }

  // Create graphics pipeline
  if (!ctx.createGraphicsPipeline(presenter.extent())) {
    std::cerr << "Failed to create graphics pipeline\n";
    cleanup();
    return 1;
  }

  // Create frame buffers
  if (!ctx.createFrameBuffers(presenter.imageViews(), presenter.extent())) {
    std::cerr << "Failed to create framebuffers\n";
    cleanup();
    return 1;
  }

  if (!ctx.createCommandPool()) {
    std::cerr << "Failed to create the command pool\n";
    cleanup();
    return 1;
  }

  if (!ctx.allocateCommandBuffers(
          static_cast<uint32_t>(ctx.framebuffers().size()))) {
    std::cerr << "Failed to allocate " << ctx.framebuffers().size()
              << " frame buffers";
    cleanup();
    return 1;
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    vkWaitForFences(ctx.device(), 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(ctx.device(), presenter.swapchain(), UINT64_MAX,
                          imageAvailable[currentFrame], VK_NULL_HANDLE,
                          &imageIndex);

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
      vkWaitForFences(ctx.device(), 1, &imagesInFlight[imageIndex], VK_TRUE,
                      UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    vkResetFences(ctx.device(), 1, &inFlightFences[currentFrame]);

    VkCommandBuffer cmd = ctx.commandBuffers()[imageIndex];
    vkResetCommandBuffer(cmd, 0);
    recordCommandBuffer(cmd, ctx.renderPass(), ctx.framebuffers()[imageIndex],
                        presenter.extent(), ctx.graphicsPipeline());

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable[currentFrame];
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinished[imageIndex];

    vkQueueSubmit(ctx.graphicsQueue(), 1, &submit,
                  inFlightFences[currentFrame]);

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished[imageIndex];
    VkSwapchainKHR sc = presenter.swapchain();
    present.swapchainCount = 1;
    present.pSwapchains = &sc;
    present.pImageIndices = &imageIndex;

    vkQueuePresentKHR(ctx.graphicsQueue(), &present);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  cleanup();
  return 0;
}
