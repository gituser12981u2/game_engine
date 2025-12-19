#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkGraphicsPipeline {
public:
  VkGraphicsPipeline() = default;
  ~VkGraphicsPipeline() noexcept { shutdown(); }

  VkGraphicsPipeline(const VkGraphicsPipeline &) = delete;
  VkGraphicsPipeline &operator=(const VkGraphicsPipeline &) = delete;

  VkGraphicsPipeline(VkGraphicsPipeline &&other) noexcept {
    *this = std::move(other);
  }
  VkGraphicsPipeline &operator=(VkGraphicsPipeline &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_pipelineLayout = std::exchange(other.m_pipelineLayout, VK_NULL_HANDLE);
    m_graphicsPipeline =
        std::exchange(other.m_graphicsPipeline, VK_NULL_HANDLE);

    return *this;
  }

  bool init(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
            const std::string &vertSpvPath, const std::string &fragSpvPath);
  void shutdown() noexcept;

  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return m_graphicsPipeline;
  }
  [[nodiscard]] VkPipelineLayout layout() const noexcept {
    return m_pipelineLayout;
  }
  [[nodiscard]] bool valid() const noexcept {
    return m_pipelineLayout != VK_NULL_HANDLE;
  }

private:
  [[nodiscard]] bool createPipelineLayout();
  [[nodiscard]] bool
  createGraphicsPipeline(VkRenderPass renderPass,
                         const VkPipelineShaderStageCreateInfo *stages,
                         uint32_t stageCount);

  VkDevice m_device = VK_NULL_HANDLE;                 // non-owning
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE; // owning
  VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;     // owning
};
