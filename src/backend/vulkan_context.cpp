#include "vulkan_context.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace {

#ifndef NDEBUG
const bool kEnableValidationLayers = true;
#else
const bool kEnableValidationLayers = false;
#endif

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset"
#endif
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  (void)messageSeverity;
  (void)messageTypes;
  (void)pUserData;

  std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n";
  return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT messenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

  if (func != nullptr) {
    func(instance, messenger, pAllocator);
  }
}

// For now, just return the extensions we know we need.
// Later you will merge this with glfwGetRequiredInstanceExtensions().
std::vector<const char *>
getRequiredExtensions(bool enableValidationLayers,
                      const std::vector<const char *> &platformExtensions) {
  std::vector<const char *> extensions;
  extensions.reserve(platformExtensions.size() + 4);

  // GLFW instance extensions
  for (const char *ext : platformExtensions) {
    extensions.push_back(ext);
  }

// MoltenVK / macOS portability.
#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

  // Debug utils for validation messages.
  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

} // namespace

VulkanContext::VulkanContext()
    : m_instance(VK_NULL_HANDLE), m_debugMessenger(VK_NULL_HANDLE),
      m_enableValidationLayers(kEnableValidationLayers) {}

VulkanContext::~VulkanContext() { shutdown(); }

bool VulkanContext::init(const std::vector<const char *> &platformExtensions) {
  if (m_enableValidationLayers && !checkValidationLayerSupport()) {
    std::cerr << "Requested validation layers not available\n";
    return false;
  }

  if (!createInstance(platformExtensions)) {
    return false;
  }

  if (m_enableValidationLayers) {
    if (!setupDebugMessenger()) {
      std::cerr << "Failed to set up debug messenger\n";
      // Non fatal
    }
  }

  if (!pickPhysicalDevice()) {
    std::cerr << "Failed to find a suitable physical device\n";
    return false;
  }

  if (!createLogicalDevice()) {
    std::cerr << "Failed to create a logical device\n";
    return false;
  }

  return true;
}

void VulkanContext::shutdown() {
  if (m_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_device);
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }

  if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }
}

bool VulkanContext::checkValidationLayerSupport() {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayer(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayer.data());

  for (const char *layerName : kValidationLayers) {
    bool found = false;
    for (const auto &layerProperties : availableLayer) {
      if (std::strcmp(layerName, layerProperties.layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::cerr << "Validation layer not found:" << layerName << "\n";
      return false;
    }
  }
  return true;
}

VulkanContext::QueueFamilyIndices
VulkanContext::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto &q = queueFamilies[i];

    if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      // TODO maybe look for compute queue or dedicated transfer
      break;
    }
  }

  return indices;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) {
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
  }

  return requiredExtensions.empty();
}

bool VulkanContext::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    std::cerr << "No Vulkan-capable GPUs found\n";
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

  for (VkPhysicalDevice device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    // TODO: Use scoring function to pick best graphics (i.e discrete >
    // integrated)

    if (indices.isComplete() && extensionsSupported) {
      m_physicalDevice = device;
      m_graphicsQueueFamilyIndex = indices.graphicsFamily.value();
      return true;
    }
  }

  std::cerr
      << "Failed to find a GPU with required queue families and extensions\n";
  return false;
}

bool VulkanContext::createLogicalDevice() {
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

  float queuePriority = 1.0f;

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

  if (m_enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
  }

  VkResult result =
      vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDevice failed with error code: " << result << "\n";
    return false;
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0,
                   &m_graphicsQueue);
  m_graphicsQueueFamilyIndex = indices.graphicsFamily.value();

  return true;
}

bool VulkanContext::createInstance(
    const std::vector<const char *> &platformExtensions) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "3DEngine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  auto extensions =
      getRequiredExtensions(m_enableValidationLayers, platformExtensions);

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (m_enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    // Hook validation messages during instance creation
    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.pNext = nullptr;
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateInstance failed with error code: " << result << "\n";
    return false;
  }

  return true;
}

bool VulkanContext::setupDebugMessenger() {
  if (!m_enableValidationLayers) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);

  VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo,
                                                 nullptr, &m_debugMessenger);
  if (result != VK_SUCCESS) {
    std::cerr << "CreateDebugUtilsMessengerEXT failed with error code: "
              << result << "\n";
    return false;
  }

  return true;
}
