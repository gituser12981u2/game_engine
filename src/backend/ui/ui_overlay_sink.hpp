#pragma once

#include <cstdint>
#include <functional>
#include <vulkan/vulkan_core.h>

class VkBackendCtx;
class GlfwWindow;

namespace ui {

struct OverlayTarget {
  VkImageView colorView = VK_NULL_HANDLE;
  VkExtent2D extent{};
  VkFormat colorFormat = VK_FORMAT_UNDEFINED;
};

struct IOverlaySink {
public:
  using BuildFn = std::function<void()>;

  IOverlaySink() = default;
  virtual ~IOverlaySink() = default;

  IOverlaySink(const IOverlaySink &) = delete;
  IOverlaySink &operator=(const IOverlaySink &) = delete;
  IOverlaySink(IOverlaySink &&) = delete;
  IOverlaySink &operator=(IOverlaySink &&) = delete;

  virtual bool init(VkBackendCtx &ctx, GlfwWindow &window,
                    uint32_t framesInFlight, VkFormat swapchainFormat) = 0;

  virtual void shutdown(VkDevice device) = 0;

  virtual void beginFrame(const BuildFn &buildPanels) = 0;

  virtual void record(VkCommandBuffer cmd, const OverlayTarget &tgt) = 0;
};

} // namespace ui
