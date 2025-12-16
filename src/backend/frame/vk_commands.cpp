#include "vk_commands.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool VkCommands::init(VkDevice device, uint32_t queueFamilyIndex,
                      VkCommandPoolCreateFlags flags) {
  if (device == VK_NULL_HANDLE) {
    std::cerr << "[Cmd] Device is null\n";
    return false;
  }

  // Re-init
  shutdown();

  m_device = device;

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = flags; // per-frame

  VkResult res = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_pool);
  if (res != VK_SUCCESS) {
    std::cerr << "[Cmd] vkCreateCommandPool failed: " << res << "\n";
    m_pool = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[Cmd] Command pool created\n";
  return true;
}

bool VkCommands::allocate(uint32_t count, VkCommandBufferLevel level) {
  if (m_device == VK_NULL_HANDLE || m_pool == VK_NULL_HANDLE) {
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

  VkResult res =
      vkAllocateCommandBuffers(m_device, &allocInfo, m_buffers.data());
  if (res != VK_SUCCESS) {
    std::cerr << "[Cmd] vkAllocateCommandBuffers failed: " << res << "\n";
    m_buffers.clear();
    return false;
  }

  std::cout << "[Cmd] Allocated " << m_buffers.size() << " command buffers\n";
  return true;
}

void VkCommands::free() noexcept {
  if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE &&
      !m_buffers.empty()) {
    vkFreeCommandBuffers(m_device, m_pool,
                         static_cast<uint32_t>(m_buffers.size()),
                         m_buffers.data());
  }
  m_buffers.clear();
}

void VkCommands::shutdown() noexcept {
  free();

  if (m_device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(m_device, m_pool, nullptr);
  }

  m_pool = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
}
