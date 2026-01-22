#pragma once

#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkShaderInterface {
public:
  VkShaderInterface() = default;
  ~VkShaderInterface() noexcept { shutdown(); }

  VkShaderInterface(const VkShaderInterface &) = delete;
  VkShaderInterface &operator=(const VkShaderInterface &) = delete;

  VkShaderInterface(VkShaderInterface &&o) noexcept { *this = std::move(o); }
  VkShaderInterface &operator=(VkShaderInterface &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_setLayoutScene = std::exchange(other.m_setLayoutScene, VK_NULL_HANDLE);
    m_setLayoutMaterial =
        std::exchange(other.m_setLayoutMaterial, VK_NULL_HANDLE);
    m_pipelineLayout = std::exchange(other.m_pipelineLayout, VK_NULL_HANDLE);

    return *this;
  }

  bool init(VkDevice device, uint32_t maxTexSrgb, uint32_t maxTexLinear);
  void shutdown() noexcept;

  [[nodiscard]] VkDescriptorSetLayout setLayoutScene() const noexcept {
    return m_setLayoutScene;
  } // set=0
  [[nodiscard]] VkDescriptorSetLayout setLayoutMaterial() const noexcept {
    return m_setLayoutMaterial;
  } // set=1

  [[nodiscard]] VkPipelineLayout pipelineLayout() const noexcept {
    return m_pipelineLayout;
  }
  [[nodiscard]] bool valid() const noexcept {
    return m_pipelineLayout != VK_NULL_HANDLE;
  }

  [[nodiscard]] uint32_t maxTexSrgb() const noexcept { return m_maxTexSrgb; }
  [[nodiscard]] uint32_t maxTexLinear() const noexcept {
    return m_maxTexLinear;
  }

private:
  VkDevice m_device = VK_NULL_HANDLE;                         // non-owning
  VkDescriptorSetLayout m_setLayoutScene = VK_NULL_HANDLE;    // owning
  VkDescriptorSetLayout m_setLayoutMaterial = VK_NULL_HANDLE; // owning
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;         // owning

  uint32_t m_maxTexSrgb = 0;
  uint32_t m_maxTexLinear = 0;
};
