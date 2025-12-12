#include "backend/vulkan_context.hpp"
#include "vulkan_presenter.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
  GLFWwindow *window = nullptr;

  VulkanPresenter presenter;
  VulkanContext ctx;

  auto cleanup = [&]() {
    ctx.destroyCommandPool();
    ctx.destroyFramebuffers();
    presenter.shutdown(ctx);
    ctx.shutdown();

    if (window) {
      glfwDestroyWindow(window);
      window = nullptr;
    }

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
    // Rendering
  }

  cleanup();
  return 0;
}
