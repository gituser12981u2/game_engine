#include "render/rendergraph/swapchain_targets.hpp"

#include "backend/presentation/vk_presenter.hpp"
#include "engine/logging/log.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

bool SwapchainTargets::init(VkBackendCtx &ctx, VkPresenter &presenter) {
  shutdown();

  const uint32_t imageCount = presenter.imageCount();
  if (imageCount == 0) {
    LOGE("presenter.imageCount() == 0");
    return false;
  }

  if (!rebuildDepth(ctx, presenter.swapchainExtent(), imageCount)) {
    LOGE("Depth images creation failed");
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
    LOGE("presenter.imageCount() == 0");
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
    if (!m_depthImages[i].init(ctx, extent)) {
      LOGE("Depth initialization failed at index {}", i);

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
