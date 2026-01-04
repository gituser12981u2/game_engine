#include "render/rendergraph/framebuffer_cache.hpp"

#include "backend/presentation/vk_presenter.hpp"
#include "render/rendergraph/main_pass.hpp"
#include "render/rendergraph/swapchain_targets.hpp"

bool FramebufferCache::init(VkBackendCtx &ctx, VkPresenter &presenter,
                            const SwapchainTargets &targets,
                            const MainPass &mainPass) {
  shutdown();

  if (!build(ctx, presenter, targets, mainPass)) {
    shutdown();
    return false;
  }

  m_initialized = true;
  return true;
}

void FramebufferCache::shutdown() noexcept {
  m_framebuffers.shutdown();
  m_initialized = false;
}

bool FramebufferCache::rebuild(VkBackendCtx &ctx, VkPresenter &presenter,
                               const SwapchainTargets &targets,
                               const MainPass &mainPass) {
  m_framebuffers.shutdown();

  if (!build(ctx, presenter, targets, mainPass)) {
    return false;
  }

  m_initialized = true;
  return true;
}

bool FramebufferCache::build(VkBackendCtx &ctx, VkPresenter &presenter,
                             const SwapchainTargets &targets,
                             const MainPass &mainPass) {
  return m_framebuffers.init(ctx.device(), mainPass.renderPass(),
                             presenter.colorViews(), targets.depthViews(),
                             presenter.swapchainExtent());
}
