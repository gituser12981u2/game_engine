#pragma once

#include <string>
#include <vulkan/vulkan_core.h>

class VkGraphicsPipeline {
public:
  VkGraphicsPipeline() = default;
  ~VkGraphicsPipeline() { shutdown(); }

  VkGraphicsPipeline(const VkGraphicsPipeline &) = delete;
  VkGraphicsPipeline &operator=(const VkGraphicsPipeline &) = delete;

  bool init(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
            const std::string &vertSpvPath, const std::string &fragSpvPaht);
  void shutdown();

  VkPipeline pipeline() const { return m_graphicsPipeline; }
  VkPipelineLayout layout() const { return m_pipelineLayout; }
  bool valid() const { return m_pipelineLayout != VK_NULL_HANDLE; }

private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
};
