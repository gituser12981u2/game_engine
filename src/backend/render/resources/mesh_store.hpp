#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/upload/vk_buffer_uploader.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"
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
  bool init(VkBackendCtx &ctx, VkUploadContext &upload,
            UploadProfiler *profiler);
  void shutdown() noexcept;

  MeshHandle createMesh(const engine::Vertex *vertices, uint32_t vertexCount,
                        const uint32_t *indices, uint32_t indexCount);
  MeshHandle createMesh(const engine::MeshData &mesh);

  [[nodiscard]] const MeshGpu *get(MeshHandle handle) const;

  bool rebind(VkBackendCtx &ctx, VkUploadContext &upload) {
    return m_uploader.init(ctx.allocator(), &upload, m_uploaderProfiler);
  }

private:
  std::vector<MeshGpu> m_meshes;
  VkBufferUploader m_uploader;                  // non-owning;
  UploadProfiler *m_uploaderProfiler = nullptr; // non-owning
};
