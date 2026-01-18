#include "vk_presenter.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "engine/logging/log.hpp"
#include "platform/window/glfw_window.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

bool VkPresenter::init(VkBackendCtx &ctx, GlfwWindow *window, uint32_t width,
                       uint32_t height) {
  if (window == nullptr) {
    LOGE("Window is null");
    return false;
  }

  if (width == 0 || height == 0) {
    LOGE("Window width and height are 0");
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_window = window;

  if (!m_window->createVulkanSurface(m_ctx->instance(), m_surface)) {
    LOGE("Surface creation failed");
    shutdown();
    return false;
  }

  if (!m_swapchain.init(*m_ctx, m_surface, width, height)) {
    LOGE("Swapchain initialization failed");
    shutdown();
    return false;
  }

  if (!m_swapchain.createSwapchainImageViews(m_ctx->device())) {
    LOGE("Swapchain image views creation failed");
    shutdown();
    return false;
  }

  return true;
}

void VkPresenter::shutdown() noexcept {
  if (m_ctx == nullptr) {
    m_surface = VK_NULL_HANDLE;
    m_window = nullptr;
    return;
  }

  VkDevice device = m_ctx->device();
  VkInstance instance = m_ctx->instance();

  if (device != VK_NULL_HANDLE) {
    m_swapchain.shutdown(device);
  }

  if (instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
    LOGD("Destroying surface");
    vkDestroySurfaceKHR(instance, m_surface, nullptr);
  }

  m_surface = VK_NULL_HANDLE;
  m_window = nullptr;
  m_ctx = nullptr;
}

bool VkPresenter::recreateSwapchain() {
  if (!isInitialized() || m_ctx == nullptr || m_window == nullptr) {
    return false;
  }

  VkDevice device = m_ctx->device();
  VkPhysicalDevice physicalDevice = m_ctx->physicalDevice();
  if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) {
    return false;
  }

  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  m_window->framebufferSize(fbWidth, fbHeight);

  // Skip recreate while minimized
  if (fbWidth == 0 || fbHeight == 0) {
    return false;
  }

  if (!m_swapchain.init(*m_ctx, m_surface, fbWidth, fbHeight)) {
    return false;
  }

  return m_swapchain.createSwapchainImageViews(device);
}
