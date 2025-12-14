#include "vulkan_shader.hpp"

#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

bool readBinaryFile(const std::string &filePath, std::vector<char> &outData) {
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[Shader] Failed to open file: " << filePath << "\n";
    return false;
  }

  const std::streamsize fileSize = file.tellg();
  if (fileSize <= 0) {
    std::cerr << "[Shader] File is empty or unreadable: " << filePath << "\n";
    return false;
  }

  outData.resize(static_cast<size_t>(fileSize));
  file.seekg(0, std::ios::beg);
  if (!file.read(outData.data(), fileSize)) {
    std::cerr << "[Shader] Failed to read file: " << filePath << "\n";
    outData.clear();
    return false;
  }

  return true;
}

bool createShaderModuleFromFile(VkDevice device, const std::string &filePath,
                                VulkanShaderModule &outModule) {
  if (outModule.m_handle != VK_NULL_HANDLE &&
      outModule.m_device != VK_NULL_HANDLE) {
    vkDestroyShaderModule(outModule.m_device, outModule.m_handle, nullptr);
    outModule.m_handle = VK_NULL_HANDLE;
    outModule.m_device = VK_NULL_HANDLE;
  }

  std::vector<char> code;
  if (!readBinaryFile(filePath, code)) {
    std::cerr << "[Shader] Failed to read SPIR-V file: " << filePath << "\n";
    return false;
  }

  if (code.size() % 4 != 0) {
    // SPIR-V should be a multiple of 4 bytes
    std::cerr << "[Shader] SPIR-V file size not multiple of 4 bytes: "
              << filePath << " (size=" << code.size() << ")\n";
    return false;
  }

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  VkResult res =
      vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
  if (res != VK_SUCCESS) {
    std::cerr << "[Shader] vkCreateShaderModule failed for " << filePath
              << " with error " << res << "\n";
    return false;
  }

  outModule.m_device = device;
  outModule.m_handle = shaderModule;
  return true;
}
