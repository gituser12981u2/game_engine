#include "vk_commands.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "engine/logging/log.hpp"

#include <cstdint>
#include <fmt/format.h>
#include <vulkan/vulkan_core.h>

bool VkCommands::init(VkBackendCtx &ctx, VkCommandPoolCreateFlags flags) {
  shutdown();

  m_ctx = &ctx;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = m_ctx->graphicsQueueFamily();
  poolInfo.flags = flags; // per-frame

  VkResult res = vkCreateCommandPool(ctx.device(), &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    LOGE("vkCreateCommandPool failed: {}", fmt::underlying(res));
    m_pool = VK_NULL_HANDLE;
    m_ctx = VK_NULL_HANDLE;
    return false;
  }

  LOGI("Command pool created");
  return true;
}

bool VkCommands::allocate(uint32_t count, VkCommandBufferLevel level) {
  VkDevice device = m_ctx->device();

  if (m_pool == VK_NULL_HANDLE) {
    LOGE("Command pool not created");
    return false;
  }

  free();

  m_buffers.resize(count, VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_pool;
  allocInfo.level = level;
  allocInfo.commandBufferCount = count;

  VkResult res = vkAllocateCommandBuffers(device, &allocInfo, m_buffers.data());
  if (res != VK_SUCCESS) {
    LOGE("vkAllocatedCommandBuffers failed: {}", fmt::underlying(res));
    m_buffers.clear();
    return false;
  }

  LOGI("Allocated {} command buffers", m_buffers.size());
  return true;
}

void VkCommands::free() noexcept {
  if (m_ctx != nullptr && m_ctx->device() != VK_NULL_HANDLE &&
      m_pool != VK_NULL_HANDLE && !m_buffers.empty()) {
    vkFreeCommandBuffers(m_ctx->device(), m_pool,
                         static_cast<uint32_t>(m_buffers.size()),
                         m_buffers.data());
  }

  m_buffers.clear();
}

void VkCommands::shutdown() noexcept {
  free();

  if (m_ctx != nullptr && m_ctx->device() != VK_NULL_HANDLE &&
      m_pool != VK_NULL_HANDLE) {
    LOGD("Destroying command pool");
    vkDestroyCommandPool(m_ctx->device(), m_pool, nullptr);
  }

  m_pool = VK_NULL_HANDLE;
  m_ctx = nullptr;
}
