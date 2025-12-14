#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
class VkFrameManager {
public:
  VkFrameManager() = default;
  ~VkFrameManager() { shutdown(); }

  VkFrameManager(const VkFrameManager &) = delete;
  VkFrameManager &operator=(const VkFrameManager &) = delete;

  bool init(VkDevice device, uint32_t framesInFlight,
            uint32_t swapchainImageCount);
  void shutdown();

  bool resizeSwapchainImages(uint32_t swapchainImageCount);

  bool beginFrame(VkSwapchainKHR swapchain, uint32_t &outImageIndex,
                  uint64_t timeout = UINT64_MAX);
  bool submitAndPresent(VkQueue queue, VkSwapchainKHR swapchain,
                        uint32_t imageIndex, VkCommandBuffer cmd,
                        VkPipelineStageFlags waitStage =
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

  uint32_t currentFrameIndex() const { return m_currentFrame; }

private:
  bool createSyncObjects();
  void destroySyncObjects();

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
