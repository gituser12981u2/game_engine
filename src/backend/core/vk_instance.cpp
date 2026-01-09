#include "vk_instance.hpp"

#include "engine/logging/log.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

DEFINE_TU_LOGGER("Backend.Instance");
#define LOG_TU_LOGGER() ThisLogger()

#ifndef NDEBUG
constexpr bool kEnableValidationLayers = true;
#else
constexpr bool kEnableValidationLayers = false;
#endif

namespace {

constexpr std::array<const char *, 1> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  (void)messageSeverity;
  (void)messageTypes;
  (void)pUserData;

  // TODO: fix
  // LOGE("Validation layer: {}", pCallbackData->pMessage);
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
  auto *func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  if (func == nullptr) {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
  return func(instance, pCreateInfo, pAllocator, pMessenger);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT messenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto *func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
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

bool checkValidationLayerSupport() {
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
      LOGE("Validation layer not found: {}", layerName);
      return false;
    }
  }
  return true;
}

} // namespace

bool VkInstanceCtx::init(std::span<const char *const> platformExtensions,
                         bool enableValidationLayers) {
  m_enableValidationLayers = enableValidationLayers && kEnableValidationLayers;

  LOGD("Validation requested={}, effective={}", enableValidationLayers,
       m_enableValidationLayers);

  if (m_enableValidationLayers && !checkValidationLayerSupport()) {
    LOGE("Requested validation layers not available");
    return false;
  }

  if (!createInstance(platformExtensions)) {
    return false;
  }

  if (m_enableValidationLayers && !setupDebugMessenger()) {
    LOGW("Failed to setup debug messenger");
  } else {
    LOGD("Debug messenger created");
  }

  return true;
}

void VkInstanceCtx::shutdown() noexcept {
  if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
    LOGD("Debug utils messenger was destroyed");
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance != VK_NULL_HANDLE) {
    LOGD("Instance was destroyed");
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }

  m_enableValidationLayers = false;
}

bool VkInstanceCtx::setupDebugMessenger() {
  if (!m_enableValidationLayers) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);

  VkResult res = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr,
                                              &m_debugMessenger);
  if (res != VK_SUCCESS) {
    LOGE("CreateDebugUtilsMessengerEXT failed with error code: {}",
         static_cast<int>(res));
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
  appInfo.pEngineName = "Quark";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  auto extensions =
      getRequiredExtensions(m_enableValidationLayers, platformExtensions);

  LOGD("Enabled instance extensions ({}):", extensions.size());
  for (const char *ext : extensions) {
    LOGD("  {}", ext);
  }

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

  VkResult res = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (res != VK_SUCCESS) {
    LOGE("vkCreateInstance failed with error code: {}",
         static_cast<uint32_t>(res));
    return false;
  }

  return true;
}
