#include "vk_backend_ctx.hpp"

#include "engine/logging/log.hpp"

#include <cstdint>
#include <span>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

DEFINE_TU_LOGGER("Backend.Ctx");
#define LOG_TU_LOGGER() ThisLogger()

bool VkBackendCtx::init(std::span<const char *const> platformExtensions,
                        bool enableValidation) {
  shutdown();

  if (!m_instance.init(platformExtensions, enableValidation)) {
    LOGE("Failed to initialize VkInstance");
    shutdown();
    return false;
  }

  if (!m_device.init(m_instance.instance())) {
    LOGE("Failed to initialize VkDevice");
    shutdown();
    return false;
  }

  if (!createAllocator()) {
    LOGE("Failed to initialize VMA allocator");
    shutdown();
    return false;
  }

  return true;
}

void VkBackendCtx::shutdown() noexcept {
  if (m_allocator != nullptr) {
    LOGD("Destroying VMA allocator");
    vmaDestroyAllocator(m_allocator);
    m_allocator = nullptr;
  }

  m_device.shutdown();
  m_instance.shutdown();
}

bool VkBackendCtx::createAllocator() {
  VmaAllocatorCreateInfo vmaInfo{};
  vmaInfo.instance = m_instance.instance();
  vmaInfo.physicalDevice = m_device.physicalDevice();
  vmaInfo.device = m_device.device();

  const VkResult res = vmaCreateAllocator(&vmaInfo, &m_allocator);
  if (res != VK_SUCCESS) {
    LOGE("Failed to initialize vmaCreateAllocator: {}",
         static_cast<uint32_t>(res));
    m_allocator = nullptr;
    return false;
  }

  return true;
}
