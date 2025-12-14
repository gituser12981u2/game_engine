#include "vk_frame_manager.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

constexpr uint32_t kOneFence = 1;
constexpr VkBool32 kWaitAll = VK_TRUE;

// TODO: allocate command buffers per framesInFlight instead of swapchain image

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

void VkFrameManager::shutdown() {
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
      std::cerr << "[Frame] Failed to create inFligh fence\n";
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

void VkFrameManager::destroySyncObjects() {
  if (m_device == VK_NULL_HANDLE) {
    m_imageAvailable.clear();
    m_inFlightFences.clear();
    m_renderFinished.clear();
    m_imagesInFlight.clear();
    return;
  }

  for (auto s : m_imageAvailable)
    if (s)
      vkDestroySemaphore(m_device, s, nullptr);

  for (auto s : m_renderFinished)
    if (s)
      vkDestroySemaphore(m_device, s, nullptr);

  for (auto f : m_inFlightFences)
    if (f)
      vkDestroyFence(m_device, f, nullptr);

  m_imageAvailable.clear();
  m_inFlightFences.clear();
  m_renderFinished.clear();
  m_imagesInFlight.clear();
}

bool VkFrameManager::beginFrame(VkSwapchainKHR swapchain,
                                uint32_t &outImageIndex, uint64_t timeout) {
  if (m_device == VK_NULL_HANDLE)
    return false;
  if (swapchain == VK_NULL_HANDLE)
    return false;

  VkFence frameFence = m_inFlightFences[m_currentFrame];

  // Wait for CPU-frame fence
  vkWaitForFences(m_device, kOneFence, &frameFence, kWaitAll, timeout);

  // Acquire image
  VkResult acq = vkAcquireNextImageKHR(m_device, swapchain, timeout,
                                       m_imageAvailable[m_currentFrame],
                                       VK_NULL_HANDLE, &outImageIndex);

  if (m_imagesInFlight[outImageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(m_device, kOneFence, &m_imagesInFlight[outImageIndex],
                    kWaitAll, timeout);
  }

  m_imagesInFlight[outImageIndex] = frameFence;

  vkResetFences(m_device, kOneFence, &frameFence);

  return true;
}

bool VkFrameManager::submitAndPresent(VkQueue queue, VkSwapchainKHR swapchain,
                                      uint32_t imageIndex, VkCommandBuffer cmd,
                                      VkPipelineStageFlags waitStage) {
  if (m_device == VK_NULL_HANDLE)
    return false;
  if (queue == VK_NULL_HANDLE)
    return false;

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
    return false;
  }

  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &signalSem;
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain;
  present.pImageIndices = &imageIndex;

  VkResult pres = vkQueuePresentKHR(queue, &present);
  // if (pres != VK_SUCCESS) {
  //   std::cerr << "[Frame] vkQueuePresentKHR failed: " << pres << "\n";
  //   return false;
  // }

  if (pres == VK_SUCCESS) {
    return true;
  }

  if (pres == VK_SUBOPTIMAL_KHR) {
    // TODO: mark "needsRecreateSwapchain = true")
    std::cerr << "[Frame] vkQueuePresentKHR returned SUBOPTIMAL_KHR\n";
    return true;
  }

  if (pres == VK_ERROR_OUT_OF_DATE_KHR) {
    // TODO: recreate swapchain since surface changed
    std::cerr << "[Frame] vkQueuePresentKHR returned OUT_OF_DATE_KHR\n";
    return false; // or return a special code / flag
  }

  m_currentFrame = (m_currentFrame + 1) % m_framesInFlight;
  return true;
}
