#include "backend/presentation/vk_presenter.hpp"
#include "engine/app/app.hpp"
#include "engine/assets/gltf/gltf_asset.hpp"
#include "engine/camera/camera.hpp"
#include "engine/geometry/transform.hpp"
#include "platform/input/camera_controller.hpp"
#include "render/renderer.hpp"
#include "render/resources/material_system.hpp"
#include "render/resources/mesh_store.hpp"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

// TODO: fix bad API
class UploadScope {
public:
  UploadScope(Renderer &r, uint32_t frameIndex) : m_r(&r), m_ok(false) {
    m_ok = m_r->beginUpload(frameIndex);
  }
  ~UploadScope() {
    if (m_ok) {
      (void)m_r->endUpload(/*wait=*/false);
    }
  }
  explicit operator bool() const noexcept { return m_ok; }

private:
  Renderer *m_r;
  bool m_ok;
};

static void pushCubeGrid(std::vector<DrawItem> &out, MeshHandle mesh,
                         uint32_t material, uint32_t cubeCount, float spacing,
                         float t) {
  const uint32_t gridW = uint32_t(std::ceil(std::sqrt(double(cubeCount))));
  const uint32_t gridH = (cubeCount + gridW - 1) / gridW;

  out.reserve(out.size() + cubeCount);

  for (uint32_t i = 0; i < cubeCount; ++i) {
    const uint32_t x = i % gridW;
    const uint32_t z = i / gridW;

    const float fx = (float(x) - float(gridW - 1) * 0.5F) * spacing;
    const float fz = (float(z) - float(gridH - 1) * 0.5F) * spacing;

    const float r = (t * 0.7F) + (float(i) * 0.001F);

    DrawItem item{};
    item.mesh = mesh;
    item.material = material;
    item.model = engine::makeModel({fx, 0.0F, fz}, {0.0F, r, 0.0F});

    out.push_back(item);
  }
}

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

  engine::assets::GltfAsset tree;
  MeshHandle cube{};

  TextureHandle texture{};
  uint32_t material = UINT32_MAX;

  {
    UploadScope up(app.renderer(), 0);
    if (!up) {
      std::cerr << "Failed to begin upload\n";
    }

    cube = app.meshes().cube();
    engine::assets::loadGltf(app.renderer(), "assets/tree.glb", tree, opt);

    texture = app.renderer().createTextureFromFile("assets/terry.jpg", true);
    material = app.renderer().createMaterialFromTexture(texture);
  }

  std::vector<DrawItem> draw;
  // const uint32_t cubeCount = 10'000;
  // draw.reserve(cubeCount);
  draw.reserve(tree.drawItems.size() + 2);
  // draw.reserve(2);

  app.run([&](float dt) {
    controller.update(dt);
    app.renderer().setCameraUBO(
        camera.makeUbo(app.presenter().swapchainExtent()));

    const float t = (float)glfwGetTime();
    draw.clear();

    DrawItem cubeA{};
    cubeA.mesh = cube;
    cubeA.material = material;
    cubeA.model = engine::makeModel({-3, 0, 0}, {0, 0, t});
    draw.push_back(cubeA);

    DrawItem cubeB{};
    cubeB.mesh = cube;
    cubeB.material = material;
    cubeB.model = engine::makeModel({+3, 0, 0}, {0, 0, -t});
    draw.push_back(cubeB);

    draw.insert(draw.end(), tree.drawItems.begin(), tree.drawItems.end());

    // draw.clear();
    // pushCubeGrid(draw, cube, material, cubeCount, 2.5F, t);

    (void)app.renderer().drawFrame(app.presenter(), draw);
  });

  return 0;
}
