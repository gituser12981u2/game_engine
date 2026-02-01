#pragma once

#include "backend/ui/ui_overlay_sink.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;
class GlfwWindow;

namespace ui {

class VkUiOverlay {
public:
  VkUiOverlay() = default;
  ~VkUiOverlay() noexcept { shutdown(); }

  VkUiOverlay(const VkUiOverlay &) = delete;
  VkUiOverlay &operator=(const VkUiOverlay &) = delete;

  VkUiOverlay(VkUiOverlay &&other) noexcept { *this = std::move(other); }
  VkUiOverlay &operator=(VkUiOverlay &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    shutdown();
    m_ctx = other.m_ctx;
    other.m_ctx = nullptr;
    m_sink = other.m_sink;
    other.m_sink = nullptr;
    m_framesInFlight = other.m_framesInFlight;
    other.m_framesInFlight = 0;
    m_swapchainFormat = other.m_swapchainFormat;
    other.m_swapchainFormat = VK_FORMAT_UNDEFINED;
    m_inited = other.m_inited;
    other.m_inited = false;
    return *this;
  }

  // Non-owning sink; lifetime must outlive VkUiOverlay.
  void setSink(IOverlaySink *sink) noexcept { m_sink = sink; }
  [[nodiscard]] IOverlaySink *sink() noexcept { return m_sink; }
  [[nodiscard]] const IOverlaySink *sink() const noexcept { return m_sink; }

  bool init(VkBackendCtx &ctx, GlfwWindow &window, uint32_t framesInFlight,
            VkFormat swapchainFormat);

  void shutdown() noexcept;

  void beginFrame(const IOverlaySink::BuildFn &buildPanels);

  void record(VkCommandBuffer cmd, const OverlayTarget &tgt);

  // If swapchain format changes (rare), you can re-init sink here.
  bool recreateIfNeeded(GlfwWindow &window, VkFormat newSwapchainFormat);

private:
  VkBackendCtx *m_ctx = nullptr;  // non-owning
  IOverlaySink *m_sink = nullptr; // non-owning
  uint32_t m_framesInFlight = 0;
  VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;
  bool m_inited = false;
};

} // namespace ui
