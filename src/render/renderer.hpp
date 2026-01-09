#pragma once

#include "backend/frame/vk_commands.hpp"
#include "backend/frame/vk_frame_manager.hpp"
#include "backend/presentation/vk_presenter.hpp"

#include "backend/profiling/cpu_profiler.hpp"
#include "backend/profiling/profiling_logger.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/profiling/vk_gpu_profiler.hpp"

#include "render/rendergraph/main_pass.hpp"
#include "render/rendergraph/swapchain_targets.hpp"

#include "render/resources/material_gpu.hpp"
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
#include <utility>
#include <vulkan/vulkan_core.h>

class VkPresenter;
class VkBackendCtx;

struct DrawItem {
  MeshHandle mesh{};
  uint32_t material = UINT32_MAX;
  glm::mat4 model = glm::mat4(1.0F);
};

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

    m_cpuProfiler = std::move(other.m_cpuProfiler);
    m_gpuProfiler = std::move(other.m_gpuProfiler);
    m_uploadProfiler = std::move(other.m_uploadProfiler);
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

    // Rebind uploader's inside stores to this renderer's command context
    if (m_ctx != nullptr && m_ctx->device() != VK_NULL_HANDLE) {
      (void)m_resources.rebind(*m_ctx, m_uploads.statik());
      (void)m_resources.rebind(*m_ctx, m_uploads.frame());
    }

    return *this;
  }

  bool init(VkBackendCtx &ctx, VkPresenter &presenter, uint32_t framesInFlight,
            const std::string &vertSpvPath, const std::string &fragSpvPath);
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
  TextureHandle createTextureFromFile(const std::string &path, bool flipY);
  bool createTextureFromImage(const engine::ImageData &img,
                              VkTexture2D &outTex);

  uint32_t createMaterialFromTexture(TextureHandle textureHandle);
  uint32_t createMaterialFromBaseColorFactor(const glm::vec4 &factor);
  void setActiveMaterial(uint32_t materialIndex);
  bool updateMaterialGPU(uint32_t materialId, const MaterialGPU &gpu);

  bool beginUpload(uint32_t frameIndex);
  bool endUpload(bool wait);

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

  CpuProfiler m_cpuProfiler;
  VkGpuProfiler m_gpuProfiler;
  UploadProfiler m_uploadProfiler;
  profiling::FrameLogger m_profileReporter{};

  uint32_t m_framesInFlight = 0;
  VkBackendCtx *m_ctx = nullptr; // non-owning

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
