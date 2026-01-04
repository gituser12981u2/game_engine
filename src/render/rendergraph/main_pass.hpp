#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/graphics/vk_pipeline.hpp"
#include "backend/graphics/vk_render_pass.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "render/rendergraph/swapchain_attachments.hpp"

#include <vulkan/vulkan_core.h>

/// Owns the primary scene render pass and graphics pipeline
/// Rebuild if:
/// - Recreate render pass and pipeline if color format or depth format changes
class MainPass {
public:
  MainPass() = default;
  ~MainPass() noexcept { shutdown(); }

  MainPass(const MainPass &) = delete;
  MainPass &operator=(const MainPass &) = delete;

  MainPass(MainPass &&) noexcept = default;
  MainPass &operator=(MainPass &&) noexcept = default;

  bool init(VkBackendCtx &ctx, VkPresenter &presenter,
            const SwapchainTargets &targets, const VkShaderInterface &interface,
            const std::string &vertSpvPath, const std::string &fragSpvPath);
  void shutdown() noexcept;

  bool recreateIfNeeded(VkBackendCtx &ctx, VkPresenter &presenter,
                        const SwapchainTargets &targets,
                        const VkShaderInterface &interface,
                        const std::string &vertSpvPath,
                        const std::string &fragSpvPath);

  [[nodiscard]] VkRenderPass renderPass() const {
    return m_renderPass.handle();
  }
  [[nodiscard]] VkPipeline pipeline() const { return m_pipeline.pipeline(); }

  [[nodiscard]] VkFormat colorFormat() const { return m_lastColorFormat; }
  [[nodiscard]] VkFormat depthFormat() const { return m_lastDepthFormat; }

private:
  bool rebuild(VkBackendCtx &ctx, VkFormat colorFmt, VkFormat depthFmt,
               VkPipelineLayout layout, const std::string &vertSpvPath,
               const std::string &fragSpvPath);

  VkRenderPassObj m_renderPass;
  VkGraphicsPipeline m_pipeline;

  VkFormat m_lastColorFormat = VK_FORMAT_UNDEFINED;
  VkFormat m_lastDepthFormat = VK_FORMAT_UNDEFINED;
  bool m_initialized = false;
};
