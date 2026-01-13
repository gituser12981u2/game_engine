#pragma once

#include "backend/core/vk_backend_ctx.hpp"

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

/**
 * @brief Owns a VkSwapchainKHR and its associated swapchain image views.
 *
 * Responsibilities:
 * - Query swapchain support details for a physical device and surface
 * - Select surface format, present mode, and extent
 * - Create/destroy VkSwapchainKHR
 * - Retrieve swapchain images and create/destroy VkImageViews
 *
 * Presentation behavior:
 * - Prefers MAILBOX present mode when available, otherwise FIFO
 * - Prefers BGRA8 + sRGB_NONLINEAR surface formats when available
 *
 * Recreation behavior:
 * - shutdown() is idempotent
 * - destroySwapchainImageViews() is safe to call independently
 */
class VkSwapchain {
public:
  VkSwapchain() = default;
  ~VkSwapchain() = default;

  VkSwapchain(const VkSwapchain &) = delete;
  VkSwapchain &operator=(const VkSwapchain &) = delete;

  VkSwapchain(VkSwapchain &&other) noexcept { *this = std::move(other); }
  VkSwapchain &operator=(VkSwapchain &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    m_surface = std::exchange(other.m_surface, VK_NULL_HANDLE);
    m_swapchain = std::exchange(other.m_swapchain, VK_NULL_HANDLE);
    m_swapchainImages = std::exchange(other.m_swapchainImages, {});
    m_swapchainImageViews = std::exchange(other.m_swapchainImageViews, {});
    m_swapchainImageFormat =
        std::exchange(other.m_swapchainImageFormat, VK_FORMAT_UNDEFINED);
    m_swapchainExtent = std::exchange(other.m_swapchainExtent, VkExtent2D{});
    return *this;
  }

  /**
   * @brief Creates or recreates the swapchain and acquire its images
   *
   * Responsibilities:
   * - Query swapchain support
   * - Choose format, present mode, and extent
   * - Destroy existing swapchain image views
   * - Create a new VkSwapchainKHR (passing the old swapchain if present)
   * - Retrieve swapchain images
   *
   * Preconditions:
   * - ctx.device() and ctx.physicalDevice() are valid
   * - surface != VK_NULL_HANDLE
   *
   * Postconditions:
   * - m_swapchain != VK_NULL_HANDLE
   * - all image views are valid
   *
   * @return true on success; false on failure. On failure, all views are
   * destroyed and the container is cleared
   */
  bool init(VkBackendCtx &ctx, VkSurfaceKHR surface, uint32_t width,
            uint32_t height);

  /**
   * @brief Destroys all swapchain image views
   *
   * Safe to call even if no views exist
   */
  void shutdown(VkDevice device) noexcept;

  [[nodiscard]] VkSwapchainKHR swapchain() const noexcept {
    return m_swapchain;
  }
  [[nodiscard]] VkFormat swapchainImageFormat() const noexcept {
    return m_swapchainImageFormat;
  }
  [[nodiscard]] VkExtent2D swapchainExtent() const noexcept {
    return m_swapchainExtent;
  }

  bool createSwapchainImageViews(VkDevice device);
  void destroySwapchainImageViews(VkDevice device) noexcept;

  [[nodiscard]] const std::vector<VkImage> &swapchainImages() const noexcept {
    return m_swapchainImages;
  }

  [[nodiscard]] const std::vector<VkImageView> &
  swapchainImageViews() const noexcept {
    return m_swapchainImageViews;
  }

private:
  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  /**
   * @brief Queries swapchain support details for a physical device and surface
   */
  static SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                       VkSurfaceKHR surface);
  /**
   * @brief Chooses the preferred surface format from the available set
   */
  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  /**
   * @brief Chooses the preferred present mode from the available set
   */
  static VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  /**
   * @brief Chooses the swapchain extent based on surface capabilities and
   * requested dimensions
   */
  static VkExtent2D
  chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width,
                   uint32_t height);

  VkSurfaceKHR m_surface = VK_NULL_HANDLE; // non-owning
  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D m_swapchainExtent{};
};
