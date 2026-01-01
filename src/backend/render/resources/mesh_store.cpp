#include "backend/render/resources/mesh_store.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/upload/vk_upload_context.hpp"

#include <iostream>

bool MeshStore::init(VkBackendCtx &ctx, VkUploadContext &upload,
                     UploadProfiler *profiler) {
  shutdown();

  m_uploaderProfiler = profiler;

  if (!m_uploader.init(ctx.allocator(), &upload, m_uploaderProfiler)) {
    std::cerr << "[MeshStore] Failed to init uploader\n";
    shutdown();
    return false;
  }

  return true;
}

void MeshStore::shutdown() noexcept {
  for (auto &mesh : m_meshes) {
    mesh.shutdown();
  }

  m_meshes.clear();
  m_uploader.shutdown();
  m_uploaderProfiler = nullptr;
}

MeshHandle MeshStore::createMesh(const engine::Vertex *vertices,
                                 uint32_t vertexCount, const uint32_t *indices,
                                 uint32_t indexCount) {
  MeshGpu gpu{};

  if (vertices == nullptr || vertexCount == 0) {
    std::cerr << "[Renderer] createMesh vertices or vertex count are 0\n";
    return {};
  }

  const VkDeviceSize vbSize =
      VkDeviceSize(sizeof(engine::Vertex)) * vertexCount;
  if (!m_uploader.uploadToDeviceLocalBuffer(
          vertices, vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, gpu.vertex)) {
    std::cerr << "[MeshStore] vertex upload failed\n";
    return {};
  }

  gpu.vertexCount = vertexCount;

  if (indices != nullptr && indexCount > 0) {
    const VkDeviceSize ibSize = VkDeviceSize(sizeof(uint32_t)) * indexCount;
    if (!m_uploader.uploadToDeviceLocalBuffer(
            indices, ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, gpu.index)) {
      std::cerr << "[MeshStore] indice upload failed\n";
      gpu.shutdown();
      return {};
    }

    gpu.indexCount = indexCount;
    // TODO: make dynamic
    gpu.indexType = VK_INDEX_TYPE_UINT32;
  }

  m_meshes.push_back(std::move(gpu));
  return MeshHandle{static_cast<uint32_t>(m_meshes.size() - 1)};
}

MeshHandle MeshStore::createMesh(const engine::MeshData &mesh) {
  return createMesh(mesh.vertices.data(),
                    static_cast<std::uint32_t>(mesh.vertices.size()),
                    mesh.indices.empty() ? nullptr : mesh.indices.data(),
                    static_cast<std::uint32_t>(mesh.indices.size()));
}

const MeshGpu *MeshStore::get(MeshHandle handle) const {
  if (handle.id >= m_meshes.size()) {
    return nullptr;
  }

  return &m_meshes[handle.id];
}
