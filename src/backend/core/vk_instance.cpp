#include "vk_instance.hpp"

#include <cstring>
#include <iostream>
#include <span>
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

std::vector<const char *>
getRequiredExtensions(bool enableValidationLayers,
                      std::span<const char *const> platformExtensions) {
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

bool VkInstanceCtx::init(std::span<const char *const> platformExtensions,
                         bool enableValidationLayers) {
  m_enableValidationLayers = enableValidationLayers;

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

  return true;
}

void VkInstanceCtx::shutdown() {
  if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }

  m_enableValidationLayers = false;
}

bool VkInstanceCtx::checkValidationLayerSupport() {
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

bool VkInstanceCtx::setupDebugMessenger() {
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

bool VkInstanceCtx::createInstance(
    std::span<const char *const> platformExtensions) {
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

#ifdef __APPLE__
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

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
