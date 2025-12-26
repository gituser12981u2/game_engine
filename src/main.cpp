#include "backend/core/vk_backend_ctx.hpp"
#include "backend/presentation/vk_presenter.hpp"
#include "backend/render/renderer.hpp"
#include "engine/camera/camera.hpp"
#include "engine/geometry/primitives.hpp"
#include "platform/input/camera_controller.hpp"
#include "platform/window/glfw_window.hpp"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

int main() {
  VkBackendCtx ctx;
  VkPresenter presenter;
  Renderer renderer;

  auto cleanup = [&]() {
    if (ctx.device() != VK_NULL_HANDLE) {
      vkDeviceWaitIdle(ctx.device());
    }

    std::cout << "Entering cleanup\n";

    renderer.shutdown();
    presenter.shutdown();
    ctx.shutdown();
  };

  GlfwWindow window;
  if (!window.init(800, 600, "Hello Window")) {
    std::cerr << "Failed to initialize glfw window\n";
    cleanup();
    return 1;
  }

#ifndef NDEBUG
  constexpr bool enableValidation = true;
#else
  constexpr bool enableValidation = false;
#endif

  const auto platformExtensions = window.requiredVulkanExtensions();
  if (platformExtensions.empty()) {
    std::cerr << "glfwGetRequiredInstanceExtensions returned nothing\n";
    cleanup();
    return 1;
  }

  if (!ctx.init(platformExtensions, enableValidation)) {
    std::cerr << "VkBackendCtx init failed\n";
    cleanup();
    return 1;
  }

  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;

  window.framebufferSize(fbWidth, fbHeight);
  if (!presenter.init(ctx, &window, fbWidth, fbHeight)) {
    std::cerr << "Failed to initialize presenter\n";
    cleanup();
    return 1;
  }

  constexpr uint32_t framesInFlight = 2;
  if (!renderer.init(ctx, presenter, framesInFlight,
                     "shaders/bin/shader.vert.spv",
                     "shaders/bin/shader.frag.spv")) {
    std::cerr << "Failed to initialize renderer\n";
    cleanup();
    return 1;
  }

  Camera camera;
  CameraController controller(window, &camera);
  controller.enableCursorCapture(true);

  double lastTime = glfwGetTime();

  auto cubeCpu = engine::primitives::cube();
  auto cubeGpu = renderer.createMesh(
      cubeCpu.vertices.data(), (uint32_t)cubeCpu.vertices.size(),
      cubeCpu.indices.data(), (uint32_t)cubeCpu.indices.size());

  auto texture = renderer.createTextureFromFile("assets/terry.jpg", true);
  uint32_t material = renderer.createMaterialFromTexture(texture);
  renderer.setActiveMaterial(material);

  // TODO: abstract main loop from main and only keep camera and controller
  while (!window.shouldClose()) {
    window.pollEvents();

    const double now = glfwGetTime();
    float dt = static_cast<float>(now - lastTime);
    lastTime = now;

    controller.update(dt);

    renderer.setCameraUBO(camera.makeUbo(presenter.extent()));

    if (!renderer.drawFrame(presenter, cubeGpu)) {
      std::cerr << "drawFrame failed\n";
      break;
    }
  }

  cleanup();
  return 0;
}
