#include "vk_frame_manager.hpp"

#include <cstdint>
#include <ctime>
#include <iostream>
#include <vulkan/vulkan_core.h>

constexpr uint32_t kOneFence = 1;
constexpr VkBool32 kWaitAll = VK_TRUE;

bool VkFrameManager::init(VkDevice device, uint32_t framesInFlight,
                          uint32_t swapchainImageCount) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[Frame] Device is null\n";
    return false;
  }

  if (framesInFlight == 0 || swapchainImageCount == 0) {
    std::cerr << "[Frame] Invalid counts\n";
    return false;
  }

  // Re-init
  shutdown();

  m_device = device;
  m_framesInFlight = framesInFlight;
  m_swapchainImageCount = swapchainImageCount;
  m_currentFrame = 0;

  return createSyncObjects();
}

void VkFrameManager::shutdown() noexcept {
  destroySyncObjects();
  m_device = VK_NULL_HANDLE;
  m_framesInFlight = 0;
  m_swapchainImageCount = 0;
  m_currentFrame = 0;
}

bool VkFrameManager::createSyncObjects() {
  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  m_imageAvailable.assign(m_framesInFlight, VK_NULL_HANDLE);
  m_inFlightFences.assign(m_framesInFlight, VK_NULL_HANDLE);
  m_renderFinished.assign(m_swapchainImageCount, VK_NULL_HANDLE);
  m_imagesInFlight.assign(m_swapchainImageCount, VK_NULL_HANDLE);

  for (uint32_t i = 0; i < m_framesInFlight; ++i) {
    if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable[i]) !=
        VK_SUCCESS) {
      std::cerr << "[Frame] Failed to create imageAvailable semaphore\n";
      return false;
    }

    if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) !=
        VK_SUCCESS) {
      std::cerr << "[Frame] Failed to create inFlight fence\n";
      return false;
    }
  }

  for (uint32_t i = 0; i < m_swapchainImageCount; ++i) {
    if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished[i]) !=
        VK_SUCCESS) {
      std::cerr << "[Frame] Failed to create renderFinished semaphore\n";
      return false;
    }
  }

  return true;
}

void VkFrameManager::destroySyncObjects() noexcept {
  if (m_device == VK_NULL_HANDLE) {
    m_imageAvailable.clear();
    m_inFlightFences.clear();
    m_renderFinished.clear();
    m_imagesInFlight.clear();
    return;
  }

  for (VkSemaphore s : m_imageAvailable) {
    if (s != VK_NULL_HANDLE) {
      vkDestroySemaphore(m_device, s, nullptr);
    }
  }

  for (VkSemaphore s : m_renderFinished) {
    if (s != VK_NULL_HANDLE) {
      vkDestroySemaphore(m_device, s, nullptr);
    }
  }

  for (VkFence f : m_inFlightFences) {
    if (f != VK_NULL_HANDLE) {
      vkDestroyFence(m_device, f, nullptr);
    }
  }

  m_imageAvailable.clear();
  m_inFlightFences.clear();
  m_renderFinished.clear();
  m_imagesInFlight.clear();
}

VkFrameManager::FrameStatus VkFrameManager::beginFrame(VkSwapchainKHR swapchain,
                                                       uint32_t &outImageIndex,
                                                       uint64_t timeout) {
  if (m_device == VK_NULL_HANDLE) {
    return FrameStatus::Error;
  }

  if (swapchain == VK_NULL_HANDLE) {
    return FrameStatus::Error;
  }

  VkFence frameFence = m_inFlightFences[m_currentFrame];

  // Wait for CPU-frame fence
  const VkResult waitRes =
      vkWaitForFences(m_device, kOneFence, &frameFence, kWaitAll, timeout);
  if (waitRes != VK_SUCCESS) {
    std::cerr << "[Frame] vkWaitForFences failed: " << waitRes << "\n";
    return FrameStatus::Error;
  }

  // Acquire image
  const VkResult acq = vkAcquireNextImageKHR(m_device, swapchain, timeout,
                                             m_imageAvailable[m_currentFrame],
                                             VK_NULL_HANDLE, &outImageIndex);
  if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
    std::cerr << "[Frame] vkAcquireNextImageKHR returned OUT_OF_DATE\n";
    return FrameStatus::OutOfDate;
  }

  if (acq == VK_SUBOPTIMAL_KHR) {
    // TODO: signal recreate swapchain when convienent
  } else if (acq != VK_SUCCESS) {
    std::cerr << "[Frame] vkAcquireNextImageKHR failed: " << acq << "\n";
    return FrameStatus::Error;
  }

  if (outImageIndex >= m_imagesInFlight.size()) {
    std::cerr << "[Frame] imageIndex out of range\n";
    return FrameStatus::Error;
  }

  if (m_imagesInFlight[outImageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(m_device, kOneFence, &m_imagesInFlight[outImageIndex],
                    kWaitAll, timeout);
  }

  m_imagesInFlight[outImageIndex] = frameFence;
  vkResetFences(m_device, kOneFence, &frameFence);

  return (acq == VK_SUBOPTIMAL_KHR) ? FrameStatus::Suboptimal : FrameStatus::Ok;
}

VkFrameManager::FrameStatus
VkFrameManager::submitAndPresent(VkQueue queue, VkSwapchainKHR swapchain,
                                 uint32_t imageIndex, VkCommandBuffer cmd,
                                 VkPipelineStageFlags waitStage) {
  if (m_device == VK_NULL_HANDLE) {
    return FrameStatus::Error;
  }

  if (queue == VK_NULL_HANDLE) {
    return FrameStatus::Error;
  }

  VkSemaphore waitSem = m_imageAvailable[m_currentFrame];
  VkSemaphore signalSem = m_renderFinished[imageIndex];
  VkFence frameFence = m_inFlightFences[m_currentFrame];

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &waitSem;
  submit.pWaitDstStageMask = &waitStage;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmd;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &signalSem;

  VkResult sub = vkQueueSubmit(queue, kOneFence, &submit, frameFence);
  if (sub != VK_SUCCESS) {
    std::cerr << "[Frame] vkQueueSubmit failed: " << sub << "\n";
    return FrameStatus::Error;
  }

  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &signalSem;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain;
  present.pImageIndices = &imageIndex;

  VkResult pres = vkQueuePresentKHR(queue, &present);

  if (pres == VK_ERROR_OUT_OF_DATE_KHR) {
    std::cerr << "[Frame] vkQueuePresentKHR returned OUT_OF_DATE\n";
    return FrameStatus::OutOfDate;
  }

  if (pres == VK_SUBOPTIMAL_KHR) {
    static bool logged = false;
    if (!logged) {
      std::cerr << "[Frame] vkQueuePresentKHR returned SUBOPTIMAL\n";
      logged = true;
    }

    m_currentFrame = (m_currentFrame + 1) % m_framesInFlight;
    return FrameStatus::Suboptimal;
  }

  if (pres != VK_SUCCESS) {
    std::cerr << "[Frame] vkQueuePresentKHR failed: " << pres << "\n";
    return FrameStatus::Error;
  }

  static bool logged = false;
  logged = false;
  m_currentFrame = (m_currentFrame + 1) % m_framesInFlight;
  return FrameStatus::Ok;
}

bool VkFrameManager::onSwapchainRecreated(uint32_t newSwapchainImageCount) {
  if (m_device == VK_NULL_HANDLE || newSwapchainImageCount == 0) {
    return false;
  }

  for (VkSemaphore s : m_renderFinished) {
    if (s != VK_NULL_HANDLE) {
      vkDestroySemaphore(m_device, s, nullptr);
    }
  }

  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  m_swapchainImageCount = newSwapchainImageCount;
  m_renderFinished.assign(m_swapchainImageCount, VK_NULL_HANDLE);
  m_imagesInFlight.assign(m_swapchainImageCount, VK_NULL_HANDLE);

  for (uint32_t i = 0; i < m_swapchainImageCount; ++i) {
    if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished[i]) !=
        VK_SUCCESS) {
      std::cerr << "[Frame] Failed to recreate renderFinished semaphore\n";
      return false;
    }
  }

  return true;
}
