#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "render/resources/material_system.hpp"
#include "render/resources/mesh_store.hpp"
#include "render/scene/scene_data.hpp"

class VkCommands;

class ResourceStore {
public:
  bool init(VkBackendCtx &ctx, const VkShaderInterface &interface,
            SceneData &data, UploadProfiler *profiler);
  void shutdown() noexcept;

  MeshStore &meshes() { return m_meshes; }
  [[nodiscard]] const MeshStore &meshes() const { return m_meshes; }

  MaterialSystem &materials() { return m_materials; }
  [[nodiscard]] const MaterialSystem &materials() const { return m_materials; }

private:
  MeshStore m_meshes;
  MaterialSystem m_materials;
};
