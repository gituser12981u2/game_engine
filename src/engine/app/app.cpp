#include "app.hpp"

#include "engine/logging/log.hpp"

#include <GLFW/glfw3.h>
#include <functional>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool EngineApp::init(const AppConfig &cfg) {
  shutdown();

  m_cfg = cfg;

  if (!m_window.init(cfg.width, cfg.height, cfg.title)) {
    std::cerr << "[App] Failed to init window\n";
    return false;
  }

  LOG_INFO("App initialized");

  const auto platformExtensions = m_window.requiredVulkanExtensions();
  if (platformExtensions.empty()) {
    std::cerr << "[App] requiredVulkanExtensions returned nothing\n";
    shutdown();
    return false;
  }

  if (!m_ctx.init(platformExtensions, cfg.enableValidation)) {
    std::cerr << "[App] VkBackendCtx init failed\n";
    shutdown();
    return false;
  }

  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  m_window.framebufferSize(fbWidth, fbHeight);
  if (!m_presenter.init(m_ctx, &m_window, fbWidth, fbHeight)) {
    std::cerr << "[App] Presenter init failed\n";
    shutdown();
    return false;
  }

  if (!m_renderer.init(m_ctx, m_presenter, cfg.framesInFlight, cfg.vertSpvPath,
                       cfg.fragSpvPath)) {
    std::cerr << "[App] Renderer init failed\n";
    shutdown();
    return false;
  }

  m_inited = true;
  return true;
}

void EngineApp::shutdown() noexcept {
  if (m_ctx.device() != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_ctx.device());
  }

  m_renderer.shutdown();
  m_presenter.shutdown();
  m_ctx.shutdown();
  m_window.shutdown();

  m_inited = false;
}

void EngineApp::run(const std::function<void(float dt)> &tick) {
  if (!m_inited) {
    return;
  }

  double lastTime = glfwGetTime();

  while (!m_window.shouldClose()) {
    m_window.pollEvents();

    const double now = glfwGetTime();
    const float dt = static_cast<float>(now - lastTime);
    lastTime = now;

    tick(dt);
  }
}
