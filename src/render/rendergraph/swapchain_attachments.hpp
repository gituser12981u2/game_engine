#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/images/vk_depth_image.hpp"
#include "backend/presentation/vk_presenter.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;

class SwapchainTargets {
public:
  SwapchainTargets() = default;
  ~SwapchainTargets() noexcept { shutdown(); }

  SwapchainTargets(const SwapchainTargets &) = delete;
  SwapchainTargets &operator=(const SwapchainTargets &) = delete;

  SwapchainTargets(SwapchainTargets &&) noexcept = default;
  SwapchainTargets &operator=(SwapchainTargets &&) noexcept = default;

  bool init(VkBackendCtx &ctx, VkPresenter &presenter);
  void shutdown() noexcept;

  // recreates depth images if extent or imageCount changed
  bool recreateIfNeeded(VkBackendCtx &ctx, VkPresenter &presenter);

  [[nodiscard]] VkFormat depthFormat() const {
    return m_depthImages.empty() ? VK_FORMAT_UNDEFINED
                                 : m_depthImages[0].format();
  }

  [[nodiscard]] const std::vector<VkImageView> &depthViews() const {
    return m_depthViews;
  }

  // TODO: add MSAA color, velocity buffer (TAA/motion blur), HDR intermediate
  // color (bloom and tonemapping)

private:
  bool rebuildDepth(VkBackendCtx &ctx, VkExtent2D extent, uint32_t imageCount);

  std::vector<VkDepthImage> m_depthImages; // one per swapchain image
  std::vector<VkImageView> m_depthViews;   // parallel to m_depthImages

  VkExtent2D m_lastExtent{0, 0};
  uint32_t m_lastImageCount = 0;
  bool m_initialized = false;
};
