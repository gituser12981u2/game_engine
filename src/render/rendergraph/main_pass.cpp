#include "render/rendergraph/main_pass.hpp"

#include <iostream>
#include <vulkan/vulkan_core.h>

bool MainPass::init(VkBackendCtx &ctx, VkPresenter &presenter,
                    const SwapchainTargets &targets,
                    const VkShaderInterface &interface,
                    const std::string &vertSpvPath,
                    const std::string &fragSpvPath) {
  shutdown();

  const VkFormat colorFmt = presenter.colorFormat();
  const VkFormat depthFmt = targets.depthFormat();
  if (colorFmt == VK_FORMAT_UNDEFINED || depthFmt == VK_FORMAT_UNDEFINED) {
    std::cerr << "[MainPass] invalid formats (color/depth)\n";
    return false;
  }

  if (!rebuild(ctx, colorFmt, depthFmt, interface.pipelineLayout(), vertSpvPath,
               fragSpvPath)) {
    shutdown();
    return false;
  }

  m_initialized = true;
  return true;
}

void MainPass::shutdown() noexcept {
  m_pipeline.shutdown();
  m_lastColorFormat = VK_FORMAT_UNDEFINED;
  m_lastDepthFormat = VK_FORMAT_UNDEFINED;
  m_initialized = false;
}

bool MainPass::recreateIfNeeded(VkBackendCtx &ctx, VkPresenter &presenter,
                                const SwapchainTargets &targets,
                                const VkShaderInterface &interface,
                                const std::string &vertSpvPath,
                                const std::string &fragSpvPath) {
  if (!m_initialized) {
    return init(ctx, presenter, targets, interface, vertSpvPath, fragSpvPath);
  }

  const VkFormat newColorFmt = presenter.colorFormat();
  const VkFormat newDepthFmt = targets.depthFormat();

  if (newColorFmt == VK_FORMAT_UNDEFINED ||
      newDepthFmt == VK_FORMAT_UNDEFINED) {
    std::cerr << "[MainPass] Invalid formats on recreate\n";
    return false;
  }

  const bool colorChanged = (newColorFmt != m_lastColorFormat);
  const bool depthChanged = (newDepthFmt != m_lastDepthFormat);

  if (!colorChanged && !depthChanged) {
    return true;
  }

  return rebuild(ctx, newColorFmt, newDepthFmt, interface.pipelineLayout(),
                 vertSpvPath, fragSpvPath);
}

bool MainPass::rebuild(VkBackendCtx &ctx, VkFormat colorFormat,
                       VkFormat depthFormat, VkPipelineLayout layout,
                       const std::string &vertSpvPath,
                       const std::string &fragSpvPath) {

  VkDevice device = ctx.device();

  m_pipeline.shutdown();

  if (!m_pipeline.init(device, colorFormat, depthFormat, layout, vertSpvPath,
                       fragSpvPath)) {
    std::cerr << "[MainPass] graphics pipeline init failed\n";
    shutdown();
    return false;
  }

  m_lastColorFormat = colorFormat;
  m_lastDepthFormat = depthFormat;
  return true;
}
