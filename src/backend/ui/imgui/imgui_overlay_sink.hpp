#pragma once

#include "backend/ui/ui_overlay_sink.hpp"

// TODO get from presenter
#include "platform/window/glfw_window.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;

namespace ui {

class ImGuiOverlaySink final : public IOverlaySink {
public:
  ImGuiOverlaySink() = default;
  ~ImGuiOverlaySink() override { /* explicit shutdown() */ }

  bool init(VkBackendCtx &ctx, GlfwWindow &window, uint32_t framesInFlight,
            VkFormat swapchainFormat) override;

  void shutdown(VkDevice device) override;

  void beginFrame(const BuildFn &buildPanels) override;
  void record(VkCommandBuffer cmd, const OverlayTarget &tgt) override;

private:
  bool createDescriptorPool(VkDevice device) noexcept;
  void destroyDescriptorPool(VkDevice device) noexcept;

  VkDescriptorPool m_descPool = VK_NULL_HANDLE;
  uint32_t m_framesInFlight = 0;
  VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;
  bool m_initialized = false;
};

} // namespace ui
