#pragma once

#include "../frame/vk_commands.hpp"
#include "../frame/vk_frame_manager.hpp"
#include "../presentation/vk_presenter.hpp"
#include "vk_framebuffers.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"

#include <cstdint>
#include <string>
#include <vulkan/vulkan_core.h>

class VkPresenter;

class Renderer {
public:
  Renderer() = default;
  ~Renderer() { shutdown(); }

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  bool init(VkDevice device, VkQueue graphicsQueue,
            uint32_t graphicsQueueFamily, VkPresenter &presenter,
            uint32_t framesInFlight, const std::string &vertSpvPath,
            const std::string &fragSpvPath);
  void shutdown();

  bool drawFrame(VkPresenter &presenter);

  bool recreateSwapchainDependent(VkPresenter &presenter,
                                  const std::string &vertSpvPath,
                                  const std::string &fragSpvPath);

private:
  void recordFrame(VkCommandBuffer cmd, VkFramebuffer fb, VkExtent2D extent);

  VkDevice m_device = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  uint32_t m_graphicsQueueFamily = UINT32_MAX;

  uint32_t m_framesInFlight = 0;

  VkCommands m_commands;
  VkFrameManager m_frames;

  VkRenderPassObj m_renderPass;
  VkGraphicsPipeline m_pipeline;
  VkFramebuffers m_framebuffers;

  std::string m_vertPath;
  std::string m_fragPath;
};
