#pragma once

#include "../frame/vk_commands.hpp"
#include "../frame/vk_frame_manager.hpp"
#include "../presentation/vk_presenter.hpp"
#include "mesh_gpu.hpp"
#include "vk_depth_image.hpp"
#include "vk_framebuffers.hpp"
#include "vk_per_frame_uniform.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_uploader.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vulkan/vulkan_core.h>

class VkPresenter;

class Renderer {
public:
  Renderer() = default;
  ~Renderer() noexcept { shutdown(); }

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  Renderer(Renderer &&other) noexcept { *this = std::move(other); }
  Renderer &operator=(Renderer &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
    m_physicalDevice = std::exchange(other.m_physicalDevice, VK_NULL_HANDLE);
    m_graphicsQueue = std::exchange(other.m_graphicsQueue, VK_NULL_HANDLE);
    m_graphicsQueueFamily =
        std::exchange(other.m_graphicsQueueFamily, UINT32_MAX);
    m_framesInFlight = std::exchange(other.m_framesInFlight, 0U);

    m_commands = std::move(other.m_commands);
    m_frames = std::move(other.m_frames);

    // TODO: add m_camera

    m_uploader = std::move(other.m_uploader);
    m_mesh = std::move(other.m_mesh);

    // Fix uploader pointer to refer to this renderer's commands
    m_uploader.init(m_physicalDevice, m_device, m_graphicsQueue, &m_commands);

    m_renderPass = std::move(other.m_renderPass);
    m_pipeline = std::move(other.m_pipeline);
    m_framebuffers = std::move(other.m_framebuffers);

    m_vertPath = std::exchange(other.m_vertPath, {});
    m_fragPath = std::exchange(other.m_fragPath, {});
    return *this;
  }

  bool init(VkPhysicalDevice physicalDevice, VkDevice device,
            VkQueue graphicsQueue, uint32_t graphicsQueueFamily,
            VkPresenter &presenter, uint32_t framesInFlight,
            const std::string &vertSpvPath, const std::string &fragSpvPath);
  void shutdown() noexcept;

  [[nodiscard]] bool drawFrame(VkPresenter &presenter);

  bool recreateSwapchainDependent(VkPresenter &presenter,
                                  const std::string &vertSpvPath,
                                  const std::string &fragSpvPath);

private:
  bool initTestGeometry();
  void recordFrame(VkCommandBuffer cmd, VkFramebuffer fb, VkExtent2D extent);

  // Todo delete: test rotations
  float m_timeSeconds = 0.0F;

  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  uint32_t m_graphicsQueueFamily = UINT32_MAX;

  uint32_t m_framesInFlight = 0;

  VkCommands m_commands;
  VkFrameManager m_frames;

  VkPerFrameUniform m_camera;

  MeshGpu m_mesh;
  VkUploader m_uploader;

  VkRenderPassObj m_renderPass;
  VkGraphicsPipeline m_pipeline;
  VkFramebuffers m_framebuffers;

  VkDepthImage m_depth;

  std::string m_vertPath;
  std::string m_fragPath;
};
