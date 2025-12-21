#include "backend/core/vk_device.hpp"
#include "backend/core/vk_instance.hpp"
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
  VkInstanceCtx instance;
  VkDeviceCtx device;
  VkPresenter presenter;
  Renderer renderer;

  auto cleanup = [&]() {
    if (device.device() != VK_NULL_HANDLE) {
      vkDeviceWaitIdle(device.device());
    }

    std::cout << "Entering cleanup\n";

    renderer.shutdown();
    presenter.shutdown();
    device.shutdown();
    instance.shutdown();
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

  // TODO: Make a "presenter" like class that owns instance and device
  if (!instance.init(platformExtensions, enableValidation)) {
    std::cerr << "Failed to initialize Vulkan\n";
    cleanup();
    return 1;
  }

  if (!device.init(instance.instance())) {
    std::cerr << "Failed to initialize Vulkan device\n";
    cleanup();
    return 1;
  }

  // TODO: abstract instance, device, presenter, and renderer from main
  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  window.framebufferSize(fbWidth, fbHeight);
  if (!presenter.init(instance.instance(), device.physicalDevice(),
                      device.device(), window.handle(), fbWidth, fbHeight,
                      device.queues().graphicsFamily)) {
    std::cerr << "Failed to initialize presenter\n";
    cleanup();
    return 1;
  }

  constexpr uint32_t framesInFlight = 2;
  if (!renderer.init(device.physicalDevice(), device.device(),
                     device.queues().graphics, device.queues().graphicsFamily,
                     presenter, framesInFlight, "shaders/bin/shader.vert.spv",
                     "shaders/bin/shader.frag.spv")) {
    std::cerr << "Failed to initialize renderer\n";
    cleanup();
    return 1;
  }

  Camera camera;
  CameraController controller(window.handle(), &camera);
  controller.enableCursorCapture(true);

  double lastTime = glfwGetTime();

  auto cubeCpu = engine::primitives::cube();
  auto cubeGpu = renderer.createMesh(
      cubeCpu.vertices.data(), (uint32_t)cubeCpu.vertices.size(),
      cubeCpu.indices.data(), (uint32_t)cubeCpu.indices.size());

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
