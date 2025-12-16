#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkFrameManager {
public:
  VkFrameManager() = default;
  ~VkFrameManager() noexcept { shutdown(); }

  VkFrameManager(const VkFrameManager &) = delete;
  VkFrameManager &operator=(const VkFrameManager &) = delete;

  VkFrameManager(VkFrameManager &&other) noexcept { *this = std::move(other); }
  VkFrameManager &operator=(VkFrameManager &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);

    m_framesInFlight = std::exchange(other.m_framesInFlight, 0U);
    m_swapchainImageCount = std::exchange(other.m_swapchainImageCount, 0U);
    m_currentFrame = std::exchange(other.m_currentFrame, 0U);

    m_imageAvailable = std::exchange(other.m_imageAvailable, {});
    m_inFlightFences = std::exchange(other.m_inFlightFences, {});
    m_renderFinished = std::exchange(other.m_renderFinished, {});
    m_imagesInFlight = std::exchange(other.m_imagesInFlight, {});

    return *this;
  }

  enum class FrameStatus : std::uint8_t { Ok, OutOfDate, Suboptimal, Error };

  bool init(VkDevice device, uint32_t framesInFlight,
            uint32_t swapchainImageCount);
  void shutdown() noexcept;

  bool resizeSwapchainImages(uint32_t swapchainImageCount);

  FrameStatus beginFrame(VkSwapchainKHR swapchain, uint32_t &outImageIndex,
                         uint64_t timeout = UINT64_MAX);
  FrameStatus
  submitAndPresent(VkQueue queue, VkSwapchainKHR swapchain, uint32_t imageIndex,
                   VkCommandBuffer cmd,
                   VkPipelineStageFlags waitStage =
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  [[nodiscard]] uint32_t currentFrameIndex() const { return m_currentFrame; }

  [[nodiscard]] bool onSwapchainRecreated(uint32_t newSwapchainImageCount);

private:
  [[nodiscard]] bool createSyncObjects();
  void destroySyncObjects() noexcept;

  VkDevice m_device = VK_NULL_HANDLE;

  uint32_t m_framesInFlight = 0;
  uint32_t m_swapchainImageCount = 0;
  uint32_t m_currentFrame = 0;

  // Per-frame sync
  std::vector<VkSemaphore> m_imageAvailable; // size = framesInFlight
  std::vector<VkFence> m_inFlightFences;     // size = framesInFlight

  // Per-swapchain-image sync
  std::vector<VkSemaphore> m_renderFinished; // size = swapchainImageCount
  std::vector<VkFence> m_imagesInFlight;     // size = swapchainImageCount
};
