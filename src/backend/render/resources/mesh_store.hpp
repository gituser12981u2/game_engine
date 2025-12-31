#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/frame/vk_commands.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/upload/vk_buffer_uploader.hpp"
#include "engine/mesh/mesh_data.hpp"
#include "engine/mesh/vertex.hpp"
#include "mesh_gpu.hpp"

#include <cstdint>
#include <vector>

struct MeshHandle {
  uint32_t id = UINT32_MAX;
};

class MeshStore {
public:
  bool init(VkBackendCtx &ctx, class VkCommands &commands,
            UploadProfiler *profiler);
  void shutdown() noexcept;

  MeshHandle createMesh(const engine::Vertex *vertices, uint32_t vertexCount,
                        const uint32_t *indices, uint32_t indexCount);
  MeshHandle createMesh(const engine::MeshData &mesh);

  [[nodiscard]] const MeshGpu *get(MeshHandle handle) const;

  bool rebind(VkBackendCtx &ctx, VkCommands &commands) {
    return m_uploader.init(ctx.allocator(), ctx.graphicsQueue(), &commands,
                           m_uploaderProfiler);
  }

private:
  std::vector<MeshGpu> m_meshes;
  VkBufferUploader m_uploader;
  UploadProfiler *m_uploaderProfiler = nullptr; // non-owning
};
