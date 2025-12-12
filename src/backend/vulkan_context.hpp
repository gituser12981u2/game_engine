#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

class VulkanContext {
public:
  VulkanContext();
  ~VulkanContext();

  bool
  init(const std::vector<const char *>
           &platformExtensions); // Create instance, debug messenger, and device
  void shutdown();               // Destroy in reverse order

  VkInstance instance() const { return m_instance; }
  VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
  VkDevice device() const { return m_device; }
  VkQueue graphicsQueue() const { return m_graphicsQueue; }
  uint32_t graphicsQueueFamilyIndex() const {
    return m_graphicsQueueFamilyIndex;
  }

  // Render pass
  VkRenderPass renderPass() const { return m_renderPass; }
  VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }
  VkPipeline graphicsPipeline() const { return m_graphicsPipeline; }

  bool createRenderPass(VkFormat swapchainFormat);
  bool createGraphicsPipeline(VkExtent2D swapchainExtent);

  // Frame buffer
  bool createFrameBuffers(const std::vector<VkImageView> &swapchainImageViews,
                          VkExtent2D swapchainExtent);
  void destroyFramebuffers();

  const std::vector<VkFramebuffer> &framebuffers() const {
    return m_swapchainFramebuffers;
  }

  bool createCommandPool();
  void destroyCommandPool();

  bool allocateCommandBuffers(uint32_t count);
  void freeCommandBuffers();

  VkCommandPool commandPool() const { return m_commandPool; }
  const std::vector<VkCommandBuffer> &commandBuffers() const {
    return m_commandBuffers;
  }

private:
  bool checkValidationLayerSupport();
  bool createInstance(const std::vector<const char *> &platformExtensions);
  bool setupDebugMessenger();

  bool pickPhysicalDevice();
  bool createLogicalDevice();

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() const { return graphicsFamily.has_value(); }
  };

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;

  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

  std::vector<VkFramebuffer> m_swapchainFramebuffers;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_commandBuffers;

  bool m_enableValidationLayers = true; // Gated by NDEBUG in cpp
};
