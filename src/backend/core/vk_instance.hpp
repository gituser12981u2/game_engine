#pragma once

#include <span>
#include <utility>
#include <vulkan/vulkan_core.h>

/**
 * @brief Owns the Vulkan instance and optionally a debug utils messenger.
 *
 * Responsibilities:
 * - Create/destroy VkInstance with required platform extensions
 * - Optionally enable validation layers and install a VkDebugUtilsMessengerEXT
 * for runtime messages
 *
 * Validation behavior:
 * - The requested enableValidation flag is additionally gated by build config
 * NDEBUG
 * - If validation is enabled and required validation layer(s) are missing
 *   init() fails.
 *
 * Platform behavior:
 * - On Apple/MoltenVK, the portability enumeration extension/flag is enabled.
 *
 * Lifetime:
 * - init() must be called before use.
 * - shutdown() is idempotent.
 */
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

  /**
   * @brief Creates a VkInstance and optionally enables validation + debug
   * messenger.
   *
   * @param platformExtensions
   *   Platform-specific instance extensions names required by the window
   * system.
   *
   * @param enableValidation
   *   Requests validation layers / debug utils. This request is gated by build
   * config NDEBUG.
   *
   * @return true on success; false on failure. On failure, the object remains
   * in a shutdown-safe state.
   */
  bool init(std::span<const char *const> platformExtensions,
            bool enableValidation);

  /**
   * @brief Destroys the debug messenger and the VkInstance.
   */
  void shutdown() noexcept;

  [[nodiscard]] VkInstance instance() const noexcept { return m_instance; }
  [[nodiscard]] bool validationEnabled() const noexcept {
    return m_enableValidationLayers;
  }

private:
  /**
   * @brief Creates VkInstance with required extensions/layers.
   *
   * Preconditions:
   * - m_enableValidationLayers reflects the final validation decision.
   *
   * Postconditions:
   * - On success, m_instance != VK_NULL_HANDLE
   */
  [[nodiscard]] bool
  createInstance(std::span<const char *const> platformExtensions);

  /**
   * @brief Creates the VkDebugUtilsMessengerEXT for runtime validation
   * messages.
   *
   * No-op if validation is disabled.
   *
   * Postconditions.
   *
   * Postconditions:
   * - On success, m_debugMessenger != VK_NULL_HANDLE
   */
  [[nodiscard]] bool setupDebugMessenger();

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
  bool m_enableValidationLayers = true; // Gated by NDEBUG in cpp
};
