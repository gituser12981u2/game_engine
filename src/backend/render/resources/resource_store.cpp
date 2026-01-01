#include "backend/render/resources/resource_store.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"

#include <iostream>

bool ResourceStore::init(VkBackendCtx &ctx, VkUploadContext &upload,
                         const VkShaderInterface &interface,
                         UploadProfiler *profiler) {
  shutdown();

  if (!m_meshes.init(ctx, upload, profiler)) {
    std::cerr << "[ResourceStore] MeshStore init failed\n";
    shutdown();
    return false;
  }

  if (!m_materials.init(ctx, upload, interface.setLayoutMaterial(), 128,
                        profiler)) {
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
