#include "vk_presenter.hpp"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkPresenter::init(VkInstance instance, VkPhysicalDevice physicalDevice,
                       VkDevice device, GLFWwindow *window, uint32_t width,
                       uint32_t height, uint32_t graphicsQueueFamilyIndex) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[Presenter] device is null\n";
    return false;
  }

  if (window == nullptr) {
    std::cerr << "[Presenter] window is null\n";
    return false;
  }

  if (instance == VK_NULL_HANDLE) {
    std::cerr << "[Presenter] ctx.instance() is null\n";
    return false;
  }

  // Re-init
  shutdown();

  m_instance = instance;
  m_device = device;

  VkResult surfRes =
      glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
  if (surfRes != VK_SUCCESS) {
    std::cerr << "[Presenter] glfwCreateWindowSurface failed: " << surfRes
              << "\n";
    m_surface = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    return false;
  }

  if (!m_swapchain.init(physicalDevice, m_device, m_surface, width, height,
                        graphicsQueueFamilyIndex)) {
    std::cerr << "[Presenter] swapchain init failed\n";
    shutdown();
    return false;
  }

  if (!m_swapchain.createSwapchainImageViews(m_device)) {
    std::cerr << "[Presenter] swapchain image views creation failed\n";
    shutdown();
    return false;
  }

  return true;
}

void VkPresenter::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    m_swapchain.shutdown(m_device);
  }

  if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  }

  m_surface = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
  m_instance = VK_NULL_HANDLE;
}
