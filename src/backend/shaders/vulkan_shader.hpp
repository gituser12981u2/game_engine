#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

// Helper to read a binary file into memory
bool readBinaryFile(const std::string &filePath, std::vector<char> &outData);

struct VulkanShaderModule {
  VkDevice m_device = VK_NULL_HANDLE;
  VkShaderModule m_handle = VK_NULL_HANDLE;

  VulkanShaderModule() = default;

  ~VulkanShaderModule() {
    if (m_handle != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
      vkDestroyShaderModule(m_device, m_handle, nullptr);
    }
  }

  // Non-copyable, non-movable
  VulkanShaderModule(const VulkanShaderModule &) = delete;
  VulkanShaderModule &operator=(const VulkanShaderModule &) = delete;

  VulkanShaderModule(VulkanShaderModule &&) = delete;
  VulkanShaderModule &operator=(VulkanShaderModule &&) = delete;
};

bool createShaderModuleFromFile(VkDevice device, const std::string &filePath,
                                VulkanShaderModule &outModule);
