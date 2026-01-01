#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/render/resources/material_system.hpp"
#include "backend/render/resources/mesh_store.hpp"
#include "backend/resources/descriptors/vk_shader_interface.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"

class VkCommands;

class ResourceStore {
public:
  bool init(VkBackendCtx &ctx, VkUploadContext &uploader,
            const VkShaderInterface &interface, UploadProfiler *profiler);
  void shutdown() noexcept;

  MeshStore &meshes() { return m_meshes; }
  [[nodiscard]] const MeshStore &meshes() const { return m_meshes; }

  MaterialSystem &materials() { return m_materials; }
  [[nodiscard]] const MaterialSystem &materials() const { return m_materials; }

  bool rebind(VkBackendCtx &ctx, VkUploadContext &upload) {
    if (!m_meshes.rebind(ctx, upload)) {
      return false;
    }

    return m_materials.rebind(ctx, upload);
  }

private:
  MeshStore m_meshes;
  MaterialSystem m_materials;
};
