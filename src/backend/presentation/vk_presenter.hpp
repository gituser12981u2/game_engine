#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "platform/window/glfw_window.hpp"
#include "vk_swapchain.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

// Owns VulkanSwapchain and VkSurfaceKHR
class VkPresenter {
public:
  VkPresenter() = default;
  ~VkPresenter() noexcept { shutdown(); }

  VkPresenter(const VkPresenter &) = delete;
  VkPresenter &operator=(const VkPresenter &) = delete;

  VkPresenter(VkPresenter &&other) noexcept { *this = std::move(other); }
  VkPresenter &operator=(VkPresenter &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_ctx = std::exchange(other.m_ctx, nullptr);
    m_window = std::exchange(other.m_window, nullptr);
    m_surface = std::exchange(other.m_surface, VK_NULL_HANDLE);
    m_swapchain = std::move(other.m_swapchain);

    return *this;
  }

  bool init(VkBackendCtx &ctx, GlfwWindow *window, uint32_t width,
            uint32_t height);
  void shutdown() noexcept;

  [[nodiscard]] bool recreateSwapchain();

  [[nodiscard]] VkFormat colorFormat() const {
    return m_swapchain.swapchainImageFormat();
  }
  [[nodiscard]] const std::vector<VkImageView> &colorViews() const {
    return m_swapchain.swapchainImageViews();
  }
  [[nodiscard]] VkExtent2D swapchainExtent() const {
    return m_swapchain.swapchainExtent();
  }
  [[nodiscard]] VkSwapchainKHR swapchain() const {
    return m_swapchain.swapchain();
  }
  [[nodiscard]] VkSurfaceKHR surface() const { return m_surface; }

  [[nodiscard]] bool isInitialized() const {
    return m_surface != VK_NULL_HANDLE;
  }

  [[nodiscard]] uint32_t imageCount() const {
    return static_cast<uint32_t>(m_swapchain.swapchainImageViews().size());
  }

private:
  VkBackendCtx *m_ctx = nullptr;  // non-owning
  GlfwWindow *m_window = nullptr; // non-owning

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VkSwapchain m_swapchain;
};
