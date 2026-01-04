#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/graphics/vk_framebuffers.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "render/rendergraph/main_pass.hpp"
#include "render/rendergraph/swapchain_targets.hpp"

#include <vulkan/vulkan_core.h>

// TODO: switch to dynamic rendering after first post processing chain
class FramebufferCache {
public:
  FramebufferCache() = default;
  ~FramebufferCache() noexcept { shutdown(); }

  FramebufferCache(const FramebufferCache &) = delete;
  FramebufferCache &operator=(const FramebufferCache &) = delete;

  FramebufferCache(FramebufferCache &&) noexcept = default;
  FramebufferCache &operator=(FramebufferCache &&) noexcept = default;

  bool init(VkBackendCtx &ctx, VkPresenter &presenter,
            const SwapchainTargets &targets, const MainPass &mainPass);
  void shutdown() noexcept;

  bool rebuild(VkBackendCtx &ctx, VkPresenter &presenter,
               const SwapchainTargets &targets, const MainPass &mainPass);

  [[nodiscard]] VkFramebuffer at(uint32_t imageIndex) const {
    return m_framebuffers.at(imageIndex);
  }

private:
  bool build(VkBackendCtx &ctx, VkPresenter &presenter,
             const SwapchainTargets &targets, const MainPass &mainPass);

  VkFramebuffers m_framebuffers;
  bool m_initialized = false;
};
