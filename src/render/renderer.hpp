#pragma once

#include "backend/frame/vk_commands.hpp"
#include "backend/frame/vk_frame_manager.hpp"
#include "backend/presentation/vk_presenter.hpp"

#include "backend/profiling/logging/profiling_logger.hpp"
#include "backend/profiling/profilers/vk_gpu_profiler.hpp"

#include "render/rendergraph/main_pass.hpp"
#include "render/rendergraph/swapchain_targets.hpp"

#include "render/resources/material_system.hpp"
#include "render/resources/mesh_store.hpp"
#include "render/resources/resource_store.hpp"

#include "render/scene/scene_data.hpp"
#include "render/upload/upload_manager.hpp"

#include "backend/gpu/descriptors/vk_shader_interface.hpp"
#include "engine/camera/camera_ubo.hpp"

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkPresenter;
class VkBackendCtx;
class JobSystem;

struct DrawItem {
  MeshHandle mesh{};
  uint32_t material = UINT32_MAX;
  glm::mat4 model = glm::mat4(1.0F);
};

struct BatchKey {
  MeshHandle mesh;
  uint32_t material;

  bool operator==(const BatchKey &other) const noexcept {
    return mesh.id == other.mesh.id && material == other.material;
  }
};

struct BatchKeyHash {
  size_t operator()(const BatchKey &key) const noexcept {
    size_t h1 = std::hash<uint32_t>{}(key.mesh.id);
    size_t h2 = std::hash<uint32_t>{}(key.material);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};

struct Batch {
  BatchKey key{};
  // TODO: make vector or arena span
  std::span<const glm::mat4> models;
};

using BatchMap =
    std::unordered_map<BatchKey, std::vector<glm::mat4>, BatchKeyHash>;

class Renderer {
public:
  Renderer() = default;
  ~Renderer() noexcept { shutdown(); }

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  Renderer(Renderer &&other) noexcept { *this = std::move(other); }
  Renderer &operator=(Renderer &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_gpuProfiler = std::move(other.m_gpuProfiler);
    m_profileReporter = std::move(other.m_profileReporter);

    m_framesInFlight = std::exchange(other.m_framesInFlight, 0U);
    m_ctx = std::exchange(other.m_ctx, nullptr);

    m_targets = std::move(other.m_targets);
    m_interface = std::move(other.m_interface);
    m_mainPass = std::move(other.m_mainPass);

    m_commands = std::move(other.m_commands);
    m_frames = std::move(other.m_frames);
    m_scene = std::move(other.m_scene);

    m_resources = std::move(other.m_resources);

    m_vertPath = std::exchange(other.m_vertPath, {});
    m_fragPath = std::exchange(other.m_fragPath, {});
    m_cameraUbo = other.m_cameraUbo;

    return *this;
  }

  bool init(VkBackendCtx &ctx, VkPresenter &presenter, uint32_t framesInFlight,
            const std::string &vertSpvPath, const std::string &fragSpvPath,
            JobSystem &jobs);
  void shutdown() noexcept;

  [[nodiscard]] bool drawFrame(VkPresenter &presenter, MeshHandle mesh);
  [[nodiscard]] bool drawFrame(VkPresenter &presenter,
                               std::span<const DrawItem> items);

  bool recreateSwapchainDependent(VkPresenter &presenter,
                                  const std::string &vertSpvPath,
                                  const std::string &fragSpvPath);

  void setCameraUBO(const CameraUBO &ubo) { m_cameraUbo = ubo; }

  // Meshes
  MeshHandle createMesh(const engine::Vertex *vertices, uint32_t vertexCount,
                        const uint32_t *indices, uint32_t indexCount);
  MeshHandle createMesh(const engine::MeshData &mesh);
  [[nodiscard]] const MeshGpu *get(MeshHandle handle) const;

  // Materials
  TextureHandle loadTextureFromFile(const std::string &path, bool flipY,
                                    MaterialSystem::TextureUsage usage);

  uint32_t createMaterial(const MaterialSystem::MaterialDescription &desc);
  uint32_t createMaterialFromTexture(TextureHandle albedo);

  bool beginUpload(uint32_t frameIndex);
  bool endUpload(bool wait);

  bool beginStaticUploads();
  bool endStaticUploads(bool wait);

  // TODO: make PImpl
private:
  bool createDefaultMaterial() noexcept;

  void recordFrame(VkCommandBuffer cmd, VkPresenter &presenter,
                   const SwapchainTargets &targets, uint32_t imageIndex,
                   MeshHandle mesh, uint32_t material,
                   glm::vec3 pos = {0, 0, 0}, glm::vec3 rotRad = {0, 0, 0},
                   glm::vec3 scale = {1, 1, 1});
  void recordFrame(VkCommandBuffer cmd, VkPresenter &presenter,
                   const SwapchainTargets &targets, uint32_t imageIndex,
                   std::span<const DrawItem> items);

  [[nodiscard]] std::unordered_map<BatchKey, std::vector<glm::mat4>,
                                   BatchKeyHash>
  buildBatches(std::span<const DrawItem> items) const;

  void drawBatches(VkCommandBuffer cmd, uint32_t frameIndex,
                   const BatchMap &batches);

  std::vector<VkImageLayout> m_swapLayouts;

  VkGpuProfiler m_gpuProfiler;
  profiling::FrameLogger m_profileReporter{};

  uint32_t m_framesInFlight = 0;
  VkBackendCtx *m_ctx = nullptr; // non-owning
  JobSystem *m_jobs = nullptr;   // non-owning

  SwapchainTargets m_targets;
  VkShaderInterface m_interface;
  MainPass m_mainPass;

  VkCommands m_commands;
  UploadManager m_uploads;
  VkFrameManager m_frames;
  SceneData m_scene;

  ResourceStore m_resources;

  std::string m_vertPath;
  std::string m_fragPath;
  CameraUBO m_cameraUbo{};
};
