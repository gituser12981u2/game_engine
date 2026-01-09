#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/graphics/vk_pipeline.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "render/rendergraph/swapchain_targets.hpp"

#include <vulkan/vulkan_core.h>

/// Owns the primary scene and graphics pipeline
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

  [[nodiscard]] VkPipeline pipeline() const { return m_pipeline.pipeline(); }

  [[nodiscard]] VkFormat colorFormat() const { return m_lastColorFormat; }
  [[nodiscard]] VkFormat depthFormat() const { return m_lastDepthFormat; }

private:
  /// Recreate pipeline if color format or depth format changes
  bool rebuild(VkBackendCtx &ctx, VkFormat colorFormat, VkFormat depthFormat,
               VkPipelineLayout layout, const std::string &vertSpvPath,
               const std::string &fragSpvPath);

  VkGraphicsPipeline m_pipeline;

  VkFormat m_lastColorFormat = VK_FORMAT_UNDEFINED;
  VkFormat m_lastDepthFormat = VK_FORMAT_UNDEFINED;
  bool m_initialized = false;
};
