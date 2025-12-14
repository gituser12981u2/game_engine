#pragma once

#include <span>
#include <vulkan/vulkan_core.h>

class VkInstanceCtx {
public:
  VkInstanceCtx() = default;
  ~VkInstanceCtx() { shutdown(); }

  VkInstanceCtx(const VkInstanceCtx &) = delete;
  VkInstanceCtx &operator=(const VkInstanceCtx &) = delete;

  bool init(std::span<const char *const> platformExtensions,
            bool enableValidation);
  void shutdown();

  VkInstance instance() const { return m_instance; }
  bool validationEnabled() const { return m_enableValidationLayers; }

private:
  bool checkValidationLayerSupport();
  bool createInstance(std::span<const char *const> platformExtensions);
  bool setupDebugMessenger();

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
  bool m_enableValidationLayers = true; // Gated by NDEBUG in cpp
};
