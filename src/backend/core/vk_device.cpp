#include "vk_device.hpp"

#include "engine/logging/log.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
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

void logPhysicalDeviceInfo(VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(physicalDevice, &props);

  LOGI("GPU selected: '{}', apiVersion={}.{}.{} vendorID=0x{:04x} "
       "deviceID=0x{:04x}",
       props.deviceName, VK_VERSION_MAJOR(props.apiVersion),
       VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion),
       props.vendorID, props.deviceID);
}

void logEnabledDeviceExtensions() {
  LOGD("Enabled device extensions ({}):", kDeviceExtensions.size());
  for (const char *ext : kDeviceExtensions) {
    LOGI("  {}", ext);
  }
}

void logQueueFamilyProps(VkPhysicalDevice physicalDevice,
                         uint32_t familyIndex) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  std::vector<VkQueueFamilyProperties> q(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, q.data());

  if (familyIndex >= count) {
    LOGW("Queue family index {} out of range (count={})", familyIndex, count);
    return;
  }

  const VkQueueFamilyProperties &props = q[familyIndex];
  LOGI("Using graphics queue family {}: queueConut={}, flags=0x{:x}, "
       "timestampValidBits={}",
       familyIndex, props.queueCount, props.queueFlags,
       props.timestampValidBits);
}

bool supportsVulkan13(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(device, &props);

  const uint32_t api = props.apiVersion;
  return (VK_VERSION_MAJOR(api) > 1) ||
         (VK_VERSION_MAJOR(api) == 1 && VK_VERSION_MINOR(api) >= 3);
}

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
    LOGE("Missing required device extensions: ");
    for (const auto &name : requiredExtensions) {
      LOGE("{}", name);
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

bool supportsDynamicRendering(VkPhysicalDevice device) {
  VkPhysicalDeviceDynamicRenderingFeatures dyn{};
  dyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

  VkPhysicalDeviceFeatures2 feats2{};
  feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  feats2.pNext = &dyn;

  vkGetPhysicalDeviceFeatures2(device, &feats2);
  return dyn.dynamicRendering == VK_TRUE;
}

} // namespace

bool VkDeviceCtx::init(VkInstance instance) {
  if (!pickPhysicalDevice(instance)) {
    LOGE("Failed to find a suitable physical device (GPU)");
    return false;
  }

  if (!createLogicalDevice()) {
    LOGE("Failed to create a logical device");
    return false;
  }

  return true;
}

void VkDeviceCtx::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    LOGD("VkDevice destroyed");
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
    LOGE("No Vulkan-capable devices found");
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (VkPhysicalDevice device : devices) {
    if (!supportsVulkan13(device)) {
      LOGW("Skipping device: Vulkan < 1.3");
      continue;
    }

    if (!supportsDynamicRendering(device)) {
      LOGW("Skipping device: dynamicRendering not supported");
      continue;
    }

    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    // TODO: Use scoring function to pick best graphics (i.e discrete >
    // integrated)

    if (indices.isComplete() && extensionsSupported) {
      const uint32_t graphicsFamily = indices.graphicsFamily.value();

      if (!supportsTimestamps(device, graphicsFamily)) {
        LOGE("Selected device likely lacks usable timestamp support");
        continue;
      }

      m_physicalDevice = device;
      m_queues.graphicsFamily = graphicsFamily;

      logPhysicalDeviceInfo(device);
      logQueueFamilyProps(device, graphicsFamily);

      logEnabledDeviceExtensions();

      return true;
    }
  }

  LOGE("Failed to find a GPU with required queue familes and extensions");
  return false;
}

bool VkDeviceCtx::createLogicalDevice() {
  if (m_physicalDevice == VK_NULL_HANDLE) {
    LOGE("createLogicalDevice called without a selected physical device");
    return false;
  }

  QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
  if (!indices.isComplete()) {
    LOGE("Graphics queue family not found on selected device");
    return false;
  }

  float queuePriority = 1.0F;

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceVulkan12Features supVk12{};
  supVk12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

  VkPhysicalDeviceDynamicRenderingFeatures supDyn{};
  supDyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

  VkPhysicalDeviceFeatures2 supFeats2{};
  supFeats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  supFeats2.pNext = &supVk12;
  supVk12.pNext = &supDyn;

  vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supFeats2);

  const bool hasNonUniform =
      (supVk12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE);

  const bool hasPartiallyBound =
      (supVk12.descriptorBindingPartiallyBound == VK_TRUE);

  const bool hasSampledImageUAB =
      (supVk12.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE);

  const bool hasRuntimeArray = (supVk12.runtimeDescriptorArray == VK_TRUE);

  if (!hasNonUniform || !hasPartiallyBound) {
    LOGE("Bindless textures not supported on this device");
    LOGE(" shaderSampledImageArrayNonUniformIndexing = {}",
         hasNonUniform ? "YES" : "NO");
    LOGE(" descriptorBindingPartiallyBound = {}",
         hasPartiallyBound ? "YES" : "NO");
    LOGE(" descriptorBindingSampledImageUpdateAfterBind = {}",
         hasSampledImageUAB ? "YES" : "NO");
    LOGE(" runtimeDescriptorArray = {}", hasRuntimeArray ? "YES" : "NO");
    return false;
  }

  VkPhysicalDeviceVulkan12Features enVk12{};
  enVk12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  enVk12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  enVk12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
  enVk12.descriptorBindingPartiallyBound = VK_TRUE;
  enVk12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
  enVk12.runtimeDescriptorArray = hasRuntimeArray ? VK_TRUE : VK_FALSE;

  VkPhysicalDeviceDynamicRenderingFeatures enDyn{};
  enDyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  enDyn.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceFeatures2 enabledFeats2{};
  enabledFeats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  enabledFeats2.pNext = &enVk12;
  enVk12.pNext = &enDyn;

  enabledFeats2.features = VkPhysicalDeviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &enabledFeats2;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(kDeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

  VkResult res =
      vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

  if (res != VK_SUCCESS) {
    LOGE("vkCreateDevice failed with error code {}", static_cast<int>(res));
    return false;
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0,
                   &m_queues.graphics);
  m_queues.graphicsFamily = indices.graphicsFamily.value();

  LOGI("Enabled bindless features:");
  LOGI("  shaderSampledImageArrayNonUniformIndexing=YES");
  LOGI("  descriptorBindingPartiallyBound=YES");
  LOGI("  descriptorBindingSampledImageUpdateAfterBind=YES");
  LOGI("  runtimeDescriptorArray={} (optional)",
       hasRuntimeArray ? "YES" : "NO");

  return true;
}
