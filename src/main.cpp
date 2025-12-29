#include "backend/presentation/vk_presenter.hpp"
#include "backend/render/renderer.hpp"
#include "engine/app/app.hpp"
#include "engine/assets/gltf/gltf_asset.hpp"
#include "engine/camera/camera.hpp"
#include "engine/geometry/transform.hpp"
#include "platform/input/camera_controller.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

int main() {
  EngineApp app;
  AppConfig cfg{};
  cfg.title = "Hello Window";

  if (!app.init(cfg)) {
    std::cerr << "App init failed\n";
    return 1;
  }

  Camera camera;
  CameraController controller(app.window(), &camera);
  controller.enableCursorCapture(true);

  engine::assets::GltfLoadOptions opt{};
  opt.flipTexcoordV = true;
  opt.axis.yUpToZUp = true;
  opt.axis.flipAxisZ = true;

  engine::assets::GltfAsset tree;
  engine::assets::loadGltf(app.renderer(), "assets/tree.glb", tree, opt);

  auto cube = app.meshes().cube();

  auto texture = app.renderer().createTextureFromFile("assets/terry.jpg", true);
  uint32_t material = app.renderer().createMaterialFromTexture(texture);

  app.run([&](float dt) {
    controller.update(dt);
    app.renderer().setCameraUBO(
        camera.makeUbo(app.presenter().swapchainExtent()));

    const float t = (float)glfwGetTime();

    std::vector<DrawItem> draw;
    draw.reserve(tree.drawItems.size() + 2);

    DrawItem cubeA{};
    cubeA.mesh = cube;
    cubeA.material = material;
    cubeA.model = engine::makeModel({-3, 0, 0}, {0, 0, t});
    draw.push_back(cubeA);

    // TODO: add instances
    DrawItem cubeB{};
    cubeB.mesh = cube;
    cubeB.material = material;
    cubeB.model = engine::makeModel({+3, 0, 0}, {0, 0, -t});
    draw.push_back(cubeB);

    draw.insert(draw.end(), tree.drawItems.begin(), tree.drawItems.end());

    (void)app.renderer().drawFrame(app.presenter(), draw);
  });

  return 0;
}
