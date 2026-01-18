#include "render/resources/resource_store.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "render/scene/scene_data.hpp"

#include <iostream>

bool ResourceStore::init(VkBackendCtx &ctx, const VkShaderInterface &interface,
                         SceneData &data) {
  shutdown();

  if (!m_meshes.init(ctx)) {
    std::cerr << "[ResourceStore] MeshStore init failed\n";
    shutdown();
    return false;
  }

  if (!m_materials.init(ctx, interface.setLayoutMaterial(),
                        data.materialCapacity())) {
    std::cerr << "[ResourceStore] MaterialSystem init failed\n";
    shutdown();
    return false;
  }

  return true;
}

void ResourceStore::shutdown() noexcept {
  m_materials.shutdown();
  m_meshes.shutdown();
}
