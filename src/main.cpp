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

  // const uint32_t cubeCount = 10'000;
  // const float spacing = 2.5F;
  // const uint32_t gridW =
  //     static_cast<uint32_t>(std::ceil(std::sqrt((double)cubeCount)));
  // const uint32_t gridH = (cubeCount + gridW - 1) / gridW;

  app.run([&](float dt) {
    controller.update(dt);
    app.renderer().setCameraUBO(
        camera.makeUbo(app.presenter().swapchainExtent()));

    const float t = (float)glfwGetTime();

    std::vector<DrawItem> draw;
    draw.reserve(tree.drawItems.size() + 2);
    // draw.reserve(2);

    // std::vector<DrawItem> draw;
    // draw.resize(cubeCount);

    // for (uint32_t i = 0; i < cubeCount; ++i) {
    //   draw[i].mesh = cube;
    //   draw[i].material = material;
    //   draw[i].model = glm::mat4(1.0F);
    // }

    // for (uint32_t i = 0; i < cubeCount; ++i) {
    //   const uint32_t x = i % gridW;
    //   const uint32_t z = i / gridW;
    //
    //   // center grid around origin
    //   const float fx =
    //       (static_cast<float>(x) - static_cast<float>(gridW - 1) * 0.5f) *
    //       spacing;
    //   const float fz =
    //       (static_cast<float>(z) - static_cast<float>(gridH - 1) * 0.5f) *
    //       spacing;
    //
    //   // small per-cube rotation variation based on index
    //   const float r = t * 0.7F + static_cast<float>(i) * 0.001F;
    //
    //   draw[i].model = engine::makeModel({fx, 0.0F, fz}, {0.0F, r, 0.0f});
    // }

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
