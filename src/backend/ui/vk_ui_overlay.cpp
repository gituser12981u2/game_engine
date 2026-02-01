#include "backend/ui/vk_ui_overlay.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "engine/logging/log.hpp"

// TODO: get from presenter
#include "platform/window/glfw_window.hpp"

namespace ui {

bool VkUiOverlay::init(VkBackendCtx &ctx, GlfwWindow &window,
                       uint32_t framesInFlight, VkFormat swapchainFormat) {
  shutdown();

  if (framesInFlight == 0) {
    LOGE("[VkUiOverlay] framesInFlight must be > 0");
    return false;
  }
  if (swapchainFormat == VK_FORMAT_UNDEFINED) {
    LOGE("[VkUiOverlay] swapchainFormat invalid");
    return false;
  }

  m_ctx = &ctx;
  m_framesInFlight = framesInFlight;
  m_swapchainFormat = swapchainFormat;

  if (m_sink == nullptr) {
    m_inited = true;
    return true;
  }

  if (!m_sink->init(ctx, window, framesInFlight, swapchainFormat)) {
    LOGE("[VkUiOverlay] sink init failed");
    shutdown();
    return false;
  }

  m_inited = true;
  return true;
}

void VkUiOverlay::shutdown() noexcept {
  VkDevice device = VK_NULL_HANDLE;
  if (m_ctx != nullptr) {
    device = m_ctx->device();
  }

  if (m_sink != nullptr && device != VK_NULL_HANDLE) {
    m_sink->shutdown(device);
  }

  m_ctx = nullptr;
  m_framesInFlight = 0;
  m_swapchainFormat = VK_FORMAT_UNDEFINED;
  m_inited = false;
}

void VkUiOverlay::beginFrame(const IOverlaySink::BuildFn &buildPanels) {
  if (!m_inited || m_sink == nullptr) {
    return;
  }
  m_sink->beginFrame(buildPanels);
}

void VkUiOverlay::record(VkCommandBuffer cmd, const OverlayTarget &tgt) {
  if (!m_inited || m_sink == nullptr) {
    return;
  }
  m_sink->record(cmd, tgt);
}

bool VkUiOverlay::recreateIfNeeded(GlfwWindow &window,
                                   VkFormat newSwapchainFormat) {
  if (!m_inited) {
    return false;
  }
  if (newSwapchainFormat == VK_FORMAT_UNDEFINED) {
    return false;
  }
  if (newSwapchainFormat == m_swapchainFormat) {
    return true;
  }

  // Re-init sink with new format
  if (m_ctx == nullptr) {
    return false;
  }

  LOGW("[VkUiOverlay] swapchain format changed, reinitializing UI overlay");
  m_swapchainFormat = newSwapchainFormat;

  if (m_sink != nullptr) {
    VkDevice device = m_ctx->device();
    if (device != VK_NULL_HANDLE) {
      m_sink->shutdown(device);
    }
    return m_sink->init(*m_ctx, window, m_framesInFlight, m_swapchainFormat);
  }

  return true;
}

} // namespace ui
