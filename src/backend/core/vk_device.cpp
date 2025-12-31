#include "vk_device.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace {

#ifdef __APPLE__
constexpr std::array<const char *, 2> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
};
#else
constexpr std::array<const char *, 1> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#endif

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;

  [[nodiscard]] bool isComplete() const noexcept {
    return graphicsFamily.has_value();
  }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto &q = queueFamilies[i];

    if ((q.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      indices.graphicsFamily = i;
      // TODO maybe look for compute queue or dedicated transfer
      break;
    }
  }

  return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(kDeviceExtensions.begin(),
                                           kDeviceExtensions.end());

  for (const auto &ext : availableExtensions) {
    requiredExtensions.erase(ext.extensionName);
  }

  if (!requiredExtensions.empty()) {
    std::cerr << "Missing required device extensions:\n";
    for (const auto &name : requiredExtensions) {
      std::cerr << "  " << name << "\n";
    }
    return false;
  }

  return true;
}

bool supportsTimestamps(VkPhysicalDevice device, uint32_t graphicsFamily) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(device, &props);

  if (props.limits.timestampPeriod <= 0.0F) {
    return false;
  }

  // If false, timestamps can still work but treat as unsupported for
  // deterministic behavior
  if (props.limits.timestampComputeAndGraphics == VK_FALSE) {
    return true;
  }

  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> q(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, q.data());

  if (graphicsFamily >= count) {
    return false;
  }

  // Must be non-zero for timestamps to be valid in queue family
  return q[graphicsFamily].timestampValidBits != 0;
}

} // namespace

bool VkDeviceCtx::init(VkInstance instance) {
  if (!pickPhysicalDevice(instance)) {
    std::cerr << "Failed to find a suitable physical device\n";
    return false;
  }

  if (!createLogicalDevice()) {
    std::cerr << "Failed to create a logical device\n";
    return false;
  }

  return true;
}

void VkDeviceCtx::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }

  m_physicalDevice = VK_NULL_HANDLE;
  m_queues = {};
}

bool VkDeviceCtx::pickPhysicalDevice(VkInstance instance) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    std::cerr << "No Vulkan-capable GPUs found\n";
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (VkPhysicalDevice device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    // TODO: Use scoring function to pick best graphics (i.e discrete >
    // integrated)

    if (indices.isComplete() && extensionsSupported) {
      const uint32_t graphicsFamily = indices.graphicsFamily.value();

      if (!supportsTimestamps(device, graphicsFamily)) {
        std::cerr
            << "[Device] Selected GPU likely lacks usable timestamp support\n";
        continue;
      }

      m_physicalDevice = device;
      m_queues.graphicsFamily = graphicsFamily;
      return true;
    }
  }

  std::cerr
      << "Failed to find a GPU with required queue families and extensions\n";
  return false;
}

bool VkDeviceCtx::createLogicalDevice() {
  if (m_physicalDevice == VK_NULL_HANDLE) {
    std::cerr
        << "createLogicalDevice called without a selected physical device\n";
    return false;
  }

  QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
  if (!indices.isComplete()) {
    std::cerr << "Graphics queue family not found on selected device\n";
    return false;
  }

  float queuePriority = 1.0F;

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures{};
  // TODO: enable features

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(kDeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

  VkResult result =
      vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDevice failed with error code: " << result << "\n";
    return false;
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0,
                   &m_queues.graphics);
  m_queues.graphicsFamily = indices.graphicsFamily.value();

  return true;
}
