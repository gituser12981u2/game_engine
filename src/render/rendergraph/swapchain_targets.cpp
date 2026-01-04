#include "render/rendergraph/swapchain_targets.hpp"

#include "backend/presentation/vk_presenter.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool SwapchainTargets::init(VkBackendCtx &ctx, VkPresenter &presenter) {
  shutdown();

  const uint32_t imageCount = presenter.imageCount();
  if (imageCount == 0) {
    std::cerr << "[SwapchainTargets] presenter.imageCount() == 0\n";
    return false;
  }

  if (!rebuildDepth(ctx, presenter.swapchainExtent(), imageCount)) {
    std::cerr << "[SwapchainTargets] Failed to create depth images\n";
    shutdown();
    return false;
  }

  m_initialized = true;
  return true;
}

void SwapchainTargets::shutdown() noexcept {
  for (auto &depth : m_depthImages) {
    depth.shutdown();
  }

  m_depthImages.clear();
  m_depthViews.clear();

  m_lastExtent = VkExtent2D{0, 0};
  m_lastImageCount = 0;
  m_initialized = false;
}

bool SwapchainTargets::recreateIfNeeded(VkBackendCtx &ctx,
                                        VkPresenter &presenter) {
  if (!m_initialized) {
    return init(ctx, presenter);
  }

  const VkExtent2D newExtent = presenter.swapchainExtent();
  const uint32_t newImageCount = presenter.imageCount();
  if (newImageCount == 0) {
    std::cerr << "[SwapchainTargets] presenter.imageCount() == 0\n";
    return false;
  }

  const bool extentChanged = (newExtent.width != m_lastExtent.width) ||
                             (newExtent.height != m_lastExtent.height);
  const bool countChanged = (newImageCount != m_lastImageCount);

  if (!extentChanged && !countChanged) {
    return true;
  }

  return rebuildDepth(ctx, newExtent, newImageCount);
}

bool SwapchainTargets::rebuildDepth(VkBackendCtx &ctx, VkExtent2D extent,
                                    uint32_t imageCount) {
  for (auto &depth : m_depthImages) {
    depth.shutdown();
  }

  m_depthImages.clear();
  m_depthViews.clear();

  m_depthImages.resize(imageCount);
  m_depthViews.resize(imageCount);

  for (uint32_t i = 0; i < imageCount; ++i) {
    if (!m_depthImages[i].init(ctx.allocator(), ctx.physicalDevice(),
                               ctx.device(), extent)) {
      std::cerr << "[SwapchainTargets] depth init failed at index " << i
                << "\n";

      // partial cleanup
      for (auto &depth : m_depthImages) {
        depth.shutdown();
      }

      m_depthImages.clear();
      m_depthViews.clear();

      return false;
    }

    m_lastExtent = extent;
    m_lastImageCount = imageCount;
    m_depthViews[i] = m_depthImages[i].view();
  }

  return true;
}
