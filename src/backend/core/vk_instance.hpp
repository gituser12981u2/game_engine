#pragma once

#include <span>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkInstanceCtx {
public:
  VkInstanceCtx() = default;
  ~VkInstanceCtx() noexcept { shutdown(); }

  VkInstanceCtx(const VkInstanceCtx &) = delete;
  VkInstanceCtx &operator=(const VkInstanceCtx &) = delete;

  VkInstanceCtx(VkInstanceCtx &&other) noexcept { *this = std::move(other); };
  VkInstanceCtx &operator=(VkInstanceCtx &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_instance = std::exchange(other.m_instance, VK_NULL_HANDLE);
    m_debugMessenger = std::exchange(other.m_debugMessenger, VK_NULL_HANDLE);
    m_enableValidationLayers =
        std::exchange(other.m_enableValidationLayers, false);

    return *this;
  }

  bool init(std::span<const char *const> platformExtensions,
            bool enableValidation);
  void shutdown() noexcept;

  [[nodiscard]] VkInstance instance() const noexcept { return m_instance; }
  [[nodiscard]] bool validationEnabled() const noexcept {
    return m_enableValidationLayers;
  }

private:
  [[nodiscard]] bool
  createInstance(std::span<const char *const> platformExtensions);
  [[nodiscard]] bool setupDebugMessenger();

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
  bool m_enableValidationLayers = true; // Gated by NDEBUG in cpp
};
