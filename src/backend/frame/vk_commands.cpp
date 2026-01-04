#include "vk_commands.hpp"

#include "backend/core/vk_backend_ctx.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkCommands::init(VkBackendCtx &ctx, VkCommandPoolCreateFlags flags) {
  if (ctx.device() == VK_NULL_HANDLE) {
    std::cerr << "[Cmd] Device is null\n";
    return false;
  }

  uint32_t family = ctx.graphicsQueueFamily();
  if (family == UINT32_MAX) {
    std::cerr << "[Cmd] ctx.graphicsQueueFamily invalid\n";
    return false;
  }

  // Re-init
  shutdown();

  m_ctx = &ctx;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = m_ctx->graphicsQueueFamily();
  poolInfo.flags = flags; // per-frame

  VkResult res = vkCreateCommandPool(ctx.device(), &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    std::cerr << "[Cmd] vkCreateCommandPool failed: " << res << "\n";
    m_pool = VK_NULL_HANDLE;
    m_ctx = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[Cmd] Command pool created\n";
  return true;
}

bool VkCommands::allocate(uint32_t count, VkCommandBufferLevel level) {
  VkDevice device = m_ctx->device();

  if (device == VK_NULL_HANDLE || m_pool == VK_NULL_HANDLE) {
    std::cerr << "[Cmd] Device or command pool not read\n";
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
    std::cerr << "[Cmd] vkAllocateCommandBuffers failed: " << res << "\n";
    m_buffers.clear();
    return false;
  }

  std::cout << "[Cmd] Allocated " << m_buffers.size() << " command buffers\n";
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
    vkDestroyCommandPool(m_ctx->device(), m_pool, nullptr);
  }

  m_pool = VK_NULL_HANDLE;
  m_ctx = nullptr;
}
