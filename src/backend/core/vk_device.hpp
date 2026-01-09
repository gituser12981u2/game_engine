#pragma once

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

/**
 * @brief Queue bundle returned by VkDeviceCtx.
 *
 * This struct is intended to expand in the future, e.g
 * present/compute/transfer.
 *
 * Invariants :
 * - graphics != VK_NULL_HANDLE
 * - graphicsFamily != UINT32_MAX
 */
struct VkQueues {
  VkQueue graphics = VK_NULL_HANDLE;
  uint32_t graphicsFamily = UINT32_MAX;

  // TODO: add surface aware device selection
};

/**
 * @brief Owns Vulkan device (VkDevice) selection and logical device creation.
 *
 * Responsibilities:
 * - Enumerate physical devices and pick one that meets engine requested
 * criterion
 *   - Choose queue families
 *   - Create VkDevice and retrieve VkQueue handles
 *
 * Requirements:
 * - VK_KHR_swapchain on all devices
 * - VK_KHR_portability_subset on Apple/MoltenVK
 * - A graphics-capable queue family
 *
 * Lifetime:
 * - init() must be called before use.
 * - shutdown() is idempotent.
 */
class VkDeviceCtx {
public:
  VkDeviceCtx() = default;
  ~VkDeviceCtx() noexcept { shutdown(); }

  VkDeviceCtx(const VkDeviceCtx &) = delete;
  VkDeviceCtx &operator=(const VkDeviceCtx &) = delete;

  VkDeviceCtx(VkDeviceCtx &&other) noexcept { *this = std::move(other); }
  VkDeviceCtx &operator=(VkDeviceCtx &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_physicalDevice = std::exchange(other.m_physicalDevice, VK_NULL_HANDLE);
    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_queues = std::exchange(other.m_queues, VkQueues{});
    return *this;
  }

  /**
   * @brief Selects a VkPhysicalDevice and creates a VkDevice + queues.
   *
   * @param instance: A valid VkInstance used to enumerate physical devices.
   * @return true on success. On failure, this object remains in a shutdown-safe
   * state.
   */
  bool init(VkInstance instance);

  /**
   * @brief Destroys the logical device and clears stored handles.
   *
   * Safe to call multiple times.
   */
  void shutdown() noexcept;

  [[nodiscard]] VkPhysicalDevice physicalDevice() const {
    return m_physicalDevice;
  }
  [[nodiscard]] VkDevice device() const { return m_device; }
  [[nodiscard]] const VkQueues &queues() const { return m_queues; }

private:
  /**
   * @brief Finds and stores a suitable physical device and queue-family
   * indices.
   *
   * On success:
   * - m_physicalDevice is set
   * - m_queues.graphicsFamily is set
   */
  [[nodiscard]] bool pickPhysicalDevice(VkInstance instance);

  /**
   * @brief Creates the logical device for the selected physical device.
   *
   * Preconditions:
   * - m_physicalDevice is valid
   * - m_queues.graphicsFamily is valid
   *
   * On success:
   * - m_device is set
   * - m_queues.graphics is retrieved
   */
  [[nodiscard]] bool createLogicalDevice();

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
  VkQueues m_queues{};
};
