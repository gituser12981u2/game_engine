#pragma once

#include "vk_device.hpp"
#include "vk_instance.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

/**
 * @brief Owns the Vulkan backend objects required to render instance, device
 * and VMA allocator.
 *
 * This is a facade that wires together:
 * - VkInstanceCtx (VkInstance + optional validation/debug)
 * - VkDeviceCtx (VkPhysicalDevice selection + VkDevice + queues)
 * - VMA (VmaAllocator bound to the chosen instance/device)
 *
 * Lifecycle:
 * - Call init() exactly once before use.
 * - Call shutdown() when done
 *
 * Ownership (RAII):
 * - This class owns all Vulkan/VMA objects it creates.
 * - Destruction order is allocator -> device -> instance.
 *
 * Move semantics:
 * - Movable, not copyable. Moved-from objects are left in a shutdown-safe
 * state.
 */
class VkBackendCtx {
public:
  VkBackendCtx() = default;
  ~VkBackendCtx() noexcept { shutdown(); }

  VkBackendCtx(const VkBackendCtx &) = delete;
  VkBackendCtx &operator=(const VkBackendCtx &) = delete;

  VkBackendCtx(VkBackendCtx &&other) noexcept { *this = std::move(other); }
  VkBackendCtx &operator=(VkBackendCtx &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_instance = std::move(other.m_instance);
    m_device = std::move(other.m_device);
    m_allocator = std::move(other.m_allocator);

    return *this;
  }

  /**
   * @brief Initializes Vulkan instance, device, queues, and VMA allocator.
   *
   * This function is idempotent with respect to the prior initialization.
   *
   * @param platformExtensions
   *   Platform-specific instance extension names required by the window system.
   *   This should include extensions like VK_KHR_surface + platform surface
   * extension.
   *
   * @param enableValidation
   *   Whether to enable validation layers / debug utilities.
   *
   * @return true if fully initialized, false on any failure. On the failure,
   * the object is returned to a cleans shutdown state.
   */
  bool init(std::span<const char *const> platformExtensions,
            bool enableValidation);

  /**
   * @brief Destroys all owned Vulkan resources and resets handles to null.
   *
   * This function is idempotent.
   *
   * Destruction order:
   * - VMA allocator
   * - device/queues
   * - instance/debug
   */
  void shutdown() noexcept;

  [[nodiscard]] VkInstance instance() const noexcept {
    return m_instance.instance();
  }
  [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept {
    return m_device.physicalDevice();
  }
  [[nodiscard]] VkDevice device() const noexcept { return m_device.device(); }
  [[nodiscard]] VmaAllocator allocator() const noexcept { return m_allocator; }

  [[nodiscard]] const VkQueues &queues() const noexcept {
    return m_device.queues();
  }
  [[nodiscard]] uint32_t graphicsQueueFamily() const noexcept {
    return m_device.queues().graphicsFamily;
  }
  [[nodiscard]] VkQueue graphicsQueue() const noexcept {
    return m_device.queues().graphics;
  }

private:
  /**
   * @brief Creates the VMA allocator using the current instance/device.
   *
   * Preconditions:
   * - m_instance.instance() is valid
   * - m_device.device() and m_device.physicalDevice() are valid
   *
   * Postconditions:
   * - On success, m_allocator != nullptr
   * - On failure, m_allocator is reset to nullptr
   */
  [[nodiscard]] bool createAllocator();

  VkInstanceCtx m_instance;
  VkDeviceCtx m_device;
  VmaAllocator m_allocator = nullptr;
};
