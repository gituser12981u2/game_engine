#include "backend/core/vk_device.hpp"
#include "backend/core/vk_instance.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "backend/render/renderer.hpp"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

static void getFrameBufferSize(GLFWwindow *window, uint32_t &outWidth,
                               uint32_t &outHeight) {
  // Get size (in pixels) for swapchain extent
  // TODO: Make this a helper function in vulkan_swapchain
  int fbWidth = 0;
  int fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  // Some platforms can report 0 during minimize
  if (fbWidth == 0 || fbHeight == 0) {
    int winWidth = 0;
    int winHeight = 0;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    glfwWaitEvents();
    fbWidth = winWidth;
    fbHeight = winHeight;
  }

  outWidth = fbWidth > 0 ? static_cast<uint32_t>(fbWidth) : 1U;
  outHeight = fbHeight > 0 ? static_cast<uint32_t>(fbHeight) : 1U;
}

int main() {
  GLFWwindow *window = nullptr;

  VkInstanceCtx instance;
  VkDeviceCtx device;
  VkPresenter presenter;
  Renderer renderer;

  auto cleanup = [&]() {
    if (device.device() != VK_NULL_HANDLE) {
      vkDeviceWaitIdle(device.device());
    }

    renderer.shutdown();
    presenter.shutdown();
    device.shutdown();
    instance.shutdown();

    if (window) {
      glfwDestroyWindow(window);
    }

    glfwTerminate();
  };

  if (glfwInit() == GLFW_FALSE) {
    std::cerr << "Failed to initialize GLFW\n";
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(800, 600, "Hello Window", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to create window\n";
    cleanup();
    return 1;
  }

  // Get needed GLFW instance extensions
  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  if (glfwExts == nullptr || glfwExtCount == 0) {
    std::cerr << "glfwGetRequiredInstanceExtensions returned nothing\n";
    cleanup();
    return 1;
  }

  std::vector<const char *> platformExtensions(glfwExts,
                                               glfwExts + glfwExtCount);

#ifndef NDEBUG
  constexpr bool enableValidation = true;
#else
  constexpr bool enableValidation = false;
#endif

  if (!instance.init(platformExtensions, enableValidation)) {
    std::cerr << "Failed to initialize Vulkan\n";
    cleanup();
    return 1;
  }

  if (!device.init(instance.instance())) {
    std::cerr << "Failed to initialize Vulkan device\n";
    cleanup();
    return 1;
  }

  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  getFrameBufferSize(window, fbWidth, fbHeight);

  if (!presenter.init(instance.instance(), device.physicalDevice(),
                      device.device(), window, fbWidth, fbHeight,
                      device.queues().graphicsFamily)) {
    std::cerr << "Failed to initialize presenter\n";
    cleanup();
    return 1;
  }

  constexpr uint32_t framesInFlight = 2;
  if (!renderer.init(device.device(), device.queues().graphics,
                     device.queues().graphicsFamily, presenter, framesInFlight,
                     "shaders/bin/shader.vert.spv",
                     "shaders/bin/shader.frag.spv")) {
    std::cerr << "Failed to initialize renderer\n";
    cleanup();
    return 1;
  }

  while (glfwWindowShouldClose(window) == GLFW_FALSE) {
    glfwPollEvents();

    if (!renderer.drawFrame(presenter)) {
      std::cerr << "drawFrame failed\n";
      break;
    }
  }

  cleanup();
  return 0;
}
