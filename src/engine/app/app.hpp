#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "backend/render/renderer.hpp"
#include "engine/geometry/mesh_factory.hpp"
#include "platform/window/glfw_window.hpp"

#include <cstdint>
#include <functional>
#include <string>

struct AppConfig {
  uint32_t width = 800;
  uint32_t height = 600;
  const char *title = "Engine";
  uint32_t framesInFlight = 2;
  std::string vertSpvPath = "shaders/bin/shader.vert.spv";
  std::string fragSpvPath = "shaders/bin/shader.frag.spv";
  bool enableValidation =
#ifndef NDEBUG
      true;
#else
      false;
#endif
};

class EngineApp {
public:
  EngineApp() = default;
  ~EngineApp() noexcept { shutdown(); }

  EngineApp(const EngineApp &) = delete;
  EngineApp &operator=(const EngineApp &) = delete;

  EngineApp(EngineApp &&) = delete;
  EngineApp &operator=(EngineApp &&) = delete;

  bool init(const AppConfig &cfg);
  void shutdown() noexcept;

  void run(const std::function<void(float dt)> &tick);

  GlfwWindow &window() noexcept { return m_window; }
  VkBackendCtx &ctx() noexcept { return m_ctx; }
  VkPresenter &presenter() noexcept { return m_presenter; }
  Renderer &renderer() noexcept { return m_renderer; }

  MeshFactory &meshes() noexcept { return m_meshes; }
  [[nodiscard]] const MeshFactory &meshes() const noexcept { return m_meshes; }

private:
  GlfwWindow m_window;
  VkBackendCtx m_ctx;
  VkPresenter m_presenter;
  Renderer m_renderer;

  MeshFactory m_meshes{m_renderer};

  AppConfig m_cfg{};
  bool m_inited = false;
};
