#include "glfw_window.hpp"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

bool GlfwWindow::init(uint32_t width, uint32_t height, const char *title) {
  shutdown();

  if (glfwInit() == GLFW_FALSE) {
    std::cerr << "Failed to initialize GLFW\n";
    return false;
  }

  m_inited = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                              title, nullptr, nullptr);
  if (m_window == nullptr) {
    std::cerr << "Failed to create window\n";
    shutdown();
    return false;
  }

  return true;
}

void GlfwWindow::shutdown() noexcept {
  if (m_window != nullptr) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }

  if (m_inited) {
    glfwTerminate();
    m_inited = false;
  }
}

bool GlfwWindow::shouldClose() const noexcept {
  if (m_window == nullptr) {
    return true;
  }
  return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void GlfwWindow::pollEvents() const noexcept { glfwPollEvents(); }

void GlfwWindow::framebufferSize(uint32_t &outWidth,
                                 uint32_t &outHeight) const {
  // Get size (in pixels) for swapchain extent
  int fbWidth = 0;
  int fbHeight = 0;
  glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

  // Some platforms can report 0 during minimize
  if (fbWidth == 0 || fbHeight == 0) {
    int winWidth = 0;
    int winHeight = 0;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    glfwWaitEvents();
    fbWidth = winWidth;
    fbHeight = winHeight;
  }

  outWidth = fbWidth > 0 ? static_cast<uint32_t>(fbWidth) : 1U;
  outHeight = fbHeight > 0 ? static_cast<uint32_t>(fbHeight) : 1U;
}

bool GlfwWindow::createVulkanSurface(VkInstance instance,
                                     VkSurfaceKHR &outSurface) const {
  outSurface = VK_NULL_HANDLE;

  if (m_window == nullptr || instance == VK_NULL_HANDLE) {
    std::cerr << "[Window] createVulkanSurface invalid args\n";
    return false;
  }

  const VkResult res =
      glfwCreateWindowSurface(instance, m_window, nullptr, &outSurface);
  if (res != VK_SUCCESS) {
    std::cerr << "[Window] glfwCreateWindowSurface failed: " << res << "\n";
    outSurface = VK_NULL_HANDLE;
    return false;
  }

  return true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<const char *> GlfwWindow::requiredVulkanExtensions() {
  uint32_t count = 0;
  const char **exts = glfwGetRequiredInstanceExtensions(&count);
  if (exts == nullptr || count == 0) {
    std::cerr << "glfwGetRequiredInstanceExtensions returned nothing\n";
  }
  return {exts, exts + count};
}
