#include "backend/render/resources/resource_store.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/frame/vk_commands.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"

#include <iostream>

bool ResourceStore::init(VkBackendCtx &ctx, VkCommands &commands,
                         const VkShaderInterface &interface) {
  shutdown();

  if (!m_meshes.init(ctx, commands)) {
    std::cerr << "[ResourceStore] MeshStore init failed\n";
    shutdown();
    return false;
  }

  if (!m_materials.init(ctx, commands, interface.setLayoutMaterial(), 128)) {
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
