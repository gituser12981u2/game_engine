#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "platform/window/glfw_window.hpp"
#include "vk_swapchain.hpp"

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

// Owns VulkanSwapchain and VkSurfaceKHR
/**
 * @brief Owns the presentation surface (VkSurfaceKHR) and swapchain resources
 * for a window
 *
 * Responsibilities:
 * - Create/destroy the window presentation surface (VkSurfaceKHR)
 * - Create/destroy the swapchain and its image views via VkSwapchain
 * - Recreate the swapchain when the framebuffer size changes or when the
 * swapchain become out-of-date
 *
 * Typical usage:
 * - init() once after the Vulkan device and window are created
 * - On resize/out-of-date/suboptimal events, call recreateSwapchain()
 * - shutdown() on teardown (idempotent)
 *
 * Ownership:
 * - VkPresenter owns VkSurfaceKHR
 * - VkPresenter owns VkSwapchain
 * - VkBackendCtx and GlfwWindow are non-owning and must outlive VkPresenter
 *
 * Recreation behavior:
 * - recreateSwapchain() queries the current framebuffer size from the window
 * - If minimized (0x0 framebuffer), recreation is skipped and returns false
 *
 * Lifetime:
 * - init() must be called before use
 * - shutdown() is idempotent
 */
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

  /**
   * @brief Creates the window surface and initializes the swapchain
   *
   * Responsibilities:
   * - shutdown() any existing resources
   * - Create VkSurfaceKHR from the window
   * - Initialize VkSwapchain and create swapchain image views
   *
   * Preconditions:
   * - window != nullptr
   * - width > 0 and height > 0
   * - ctx is initialized
   *
   * Postconditions:
   * - m_surface != VK_NULL_HANDLE
   * - swapchain image views are created and accessible via colorViews()
   *
   *
   * @return true on success; false on failure. On failure, the object remains
   * in shutdown-safe state
   */
  bool init(VkBackendCtx &ctx, GlfwWindow *window, uint32_t width,
            uint32_t height);
  void shutdown() noexcept;

  /**
   * @brief Recreates the swapchain and its image views using the current
   * framebuffer size
   *
   * Return false if:
   * - The presenter is not initialized
   * - The window is minimized
   * - Swapchain recreation fails
   */
  [[nodiscard]] bool recreateSwapchain();

  [[nodiscard]] VkFormat colorFormat() const {
    return m_swapchain.swapchainImageFormat();
  }
  [[nodiscard]] const std::vector<VkImage> &colorImages() const {
    return m_swapchain.swapchainImages();
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
