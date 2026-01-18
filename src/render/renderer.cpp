#include "renderer.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/presentation/vk_presenter.hpp"

#include "backend/profiling/logging/profiling_logger.hpp"
#include "backend/profiling/profilers/vk_gpu_profiler.hpp"
#include "backend/profiling/telemetry/telemetry.hpp"

#include "engine/geometry/transform.hpp"
#include "engine/jobs/job_system.hpp"
#include "engine/mesh/mesh_data.hpp"

#include "render/rendergraph/swapchain_targets.hpp"
#include "render/resources/mesh_gpu.hpp"
#include "render/resources/mesh_store.hpp"
#include "render/scene/push_constants.hpp"

#include "util/scope_exit.hpp"
#include "util/vk_barrier.hpp"

#include "engine/logging/log.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

static constexpr VkDeviceSize kMiB = 1024ULL * 1024ULL;

// 8 MiB
static constexpr VkDeviceSize kUploadStaticBudget = 8ULL * kMiB;

// 2 MiB
static constexpr VkDeviceSize kUploadFrameBudget = 2ULL * kMiB;

static constexpr uint32_t kRequestedMaxInstancesPerFrame = 16U * 1024U;
static constexpr uint32_t kRequestedMaxMaterials = 1024U;

bool Renderer::init(VkBackendCtx &ctx, VkPresenter &presenter,
                    uint32_t framesInFlight, const std::string &vertSpvPath,
                    const std::string &fragSpvPath, JobSystem &jobs) {
  if (ctx.device() == VK_NULL_HANDLE ||
      ctx.physicalDevice() == VK_NULL_HANDLE ||
      ctx.graphicsQueue() == VK_NULL_HANDLE ||
      ctx.graphicsQueueFamily() == UINT32_MAX) {
    LOGE("Backend context not initialized");
    return false;
  }

  if (framesInFlight == 0) {
    LOGE("FramesInFlight must be > 0");
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_jobs = &jobs;
  m_framesInFlight = framesInFlight;
  m_vertPath = vertSpvPath;
  m_fragPath = fragSpvPath;

  LOGI("Renderer initialized: framesInFlight={} | threadCount: {} | shaders: "
       "vert='{}' frag='{}' "
       "| "
       "uploadMiB: static={} frame={} | caps: instances={} materials={}",
       framesInFlight, m_jobs->threadCount(), vertSpvPath, fragSpvPath,
       kUploadStaticBudget / kMiB, kUploadFrameBudget / kMiB,
       kRequestedMaxInstancesPerFrame, kRequestedMaxMaterials);

  VkDevice device = m_ctx->device();

  if (!m_gpuProfiler.init(*m_ctx, m_framesInFlight)) {
    LOGE("Failed to intiailize GPU profiler");
    shutdown();
    return false;
  }

  // Create swapchain targets
  if (!m_targets.init(*m_ctx, presenter)) {
    LOGE("Failed to initialize swapchain depth targets");
    shutdown();
    return false;
  }

  // Create shader interface
  if (!m_interface.init(device)) {
    LOGE("Failed to initialize shader interface");
    shutdown();
    return false;
  }

  // Create main pass
  if (!m_mainPass.init(*m_ctx, presenter, m_targets, m_interface, m_vertPath,
                       m_fragPath)) {
    LOGE("Failed to initialize main pass");
    shutdown();
    return false;
  }

  // Create command pool
  if (!m_commands.init(*m_ctx)) {
    LOGE("Failed to initialize renderer command pool");
    shutdown();
    return false;
  }
  LOGI("Main render pass initialized");

  // TODO: use job system workers instead of hard setting to 1 thread
  if (!m_uploads.init(*m_ctx, m_framesInFlight, kUploadStaticBudget,
                      kUploadFrameBudget, m_jobs->threadCount())) {
    LOGE("Failed to initialize upload manager");
    shutdown();
    return false;
  }

  if (!m_uploads.beginStatic()) {
    LOGE("Failed to begin upload frame");
    shutdown();
    return false;
  }

  if (!m_scene.init(*m_ctx, m_framesInFlight, m_interface,
                    kRequestedMaxInstancesPerFrame, kRequestedMaxMaterials)) {
    LOGE("Failed to initialize scene data");
    shutdown();
    return false;
  }

  if (!m_resources.init(*m_ctx, m_interface, m_scene)) {
    LOGE("Failed to initialize resources store");
    shutdown();
    return false;
  }

  m_resources.materials().bindMaterialTable(m_scene.materialBuffer(),
                                            m_scene.materialCapacity());

  // Create a 1x1 default white texture and material
  // TOOD: use job system worker instead of hardcoding 0
  if (!m_resources.materials().createDefaultMaterial(
          m_uploads.staticRecorder(2))) {
    LOGE("Failed to create the default material");
    shutdown();
    return false;
  }
  LOGD("Default material created");

  // Submit + wait for default material
  if (!m_uploads.flushStatic(false)) {
    LOGE("Failed to flush static uploads");
    shutdown();
    return false;
  }

  // Allocate buffer per frame
  if (!m_commands.allocate(m_framesInFlight)) {
    LOGE("Failed to allocate command buffer");
    shutdown();
    return false;
  }

  if (!m_frames.init(device, m_framesInFlight, presenter.imageCount())) {
    LOGE("Failed to initialize frame sync objects");
    shutdown();
    return false;
  }

  m_swapLayouts.assign(presenter.imageCount(), VK_IMAGE_LAYOUT_UNDEFINED);

  return true;
}

// Destroy in reverse initialization order
void Renderer::shutdown() noexcept {
  VkDevice device = VK_NULL_HANDLE;
  if (m_ctx != nullptr) {
    device = m_ctx->device();
  }

  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }

  // Commands-dependents
  m_frames.shutdown();
  m_resources.shutdown();
  m_uploads.shutdown();
  m_commands.shutdown();

  m_scene.shutdown();

  // Swapchain-dependents
  m_mainPass.shutdown();
  m_interface.shutdown();
  m_targets.shutdown();

  m_ctx = nullptr;
  m_framesInFlight = 0;
  m_gpuProfiler.shutdown();

  m_vertPath.clear();
  m_fragPath.clear();
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkPresenter &presenter,
                           const SwapchainTargets &targets, uint32_t imageIndex,
                           const MeshHandle mesh, uint32_t material,
                           glm::vec3 pos, glm::vec3 rotRad, glm::vec3 scale) {
  DrawItem item{};
  item.mesh = mesh;
  item.material = material;
  item.model = engine::makeModel(pos, rotRad, scale);

  recordFrame(cmd, presenter, targets, imageIndex,
              std::span<const DrawItem>(&item, 1));
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkPresenter &presenter,
                           const SwapchainTargets &targets, uint32_t imageIndex,
                           std::span<const DrawItem> items) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cmd, &beginInfo);

  const uint32_t frameIndex = m_frames.currentFrameIndex();
  VkExtent2D extent = presenter.swapchainExtent();

  m_gpuProfiler.beginFrameCmd(cmd, frameIndex);
  m_gpuProfiler.markFrameBegin(cmd, frameIndex);

  std::array<VkClearValue, 2> clears{};
  clears[0].color = {{0.05F, 0.05F, 0.08F, 1.0F}};
  clears[1].depthStencil =
      VkClearDepthStencilValue{.depth = 1.0F, .stencil = 0};

  VkImage scImg = presenter.colorImages()[imageIndex];
  VkImageView scView = presenter.colorViews()[imageIndex];
  VkImageView depthView = targets.depthViews()[imageIndex];

  VkImageLayout &layout = m_swapLayouts[imageIndex];

  util::cmdImageBarrier(
      cmd, scImg, layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkRenderingAttachmentInfo colorAttach{};
  colorAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttach.imageView = scView;
  colorAttach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttach.clearValue = clears[0];

  VkRenderingAttachmentInfo depthAttach{};
  depthAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depthAttach.imageView = depthView;
  depthAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttach.clearValue = clears[1];

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea =
      VkRect2D{.offset = {.x = 0, .y = 0}, .extent = extent};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttach;
  renderingInfo.pDepthAttachment = &depthAttach;

  m_gpuProfiler.markMainPassBegin(cmd, frameIndex);
  vkCmdBeginRendering(cmd, &renderingInfo);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_mainPass.pipeline());
  PROFILE_CPU_INC_PIPELINE_BINDS(1);

  // Viewport / scissor
  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = VkOffset2D{0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  m_scene.bind(cmd, m_interface, m_frames.currentFrameIndex());
  PROFILE_CPU_INC_DESCRIPTOR_BINDS(1);

  // TODO: sort by mesh, material and stream directly into the uploader
  // without building vectors per batch
  auto batches = buildBatches(items);
  drawBatches(cmd, frameIndex, batches);

  vkCmdEndRendering(cmd);
  m_gpuProfiler.markMainPassEnd(cmd, frameIndex);

  util::cmdImageBarrier(
      cmd, scImg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  m_gpuProfiler.markFrameEnd(cmd, frameIndex);
  vkEndCommandBuffer(cmd);
}

BatchMap Renderer::buildBatches(std::span<const DrawItem> items) const {
  BatchMap batches;
  batches.reserve(items.size());

  for (const DrawItem &item : items) {
    if (m_resources.meshes().get(item.mesh) == nullptr) {
      continue;
    }

    uint32_t mat = m_resources.materials().resolveMaterial(item.material);
    batches[BatchKey{.mesh = item.mesh, .material = mat}].push_back(item.model);
  }

  return batches;
}

void Renderer::drawBatches(VkCommandBuffer cmd, uint32_t frameIndex,
                           const BatchMap &batches) {

  uint32_t cursor = 0; // mat4 units within frame slice

  // TODO: parallelize batching with each job having its own workerIndex
  VkUploadContext::Recorder rec = m_uploads.frameRecorder(/*threadIndex=*/0);
  if (!rec) {
    LOGW("Frame recorder invalid (frameIndex={})", frameIndex);
    return;
  }

  for (const auto &[key, models] : batches) {
    const MeshGpu *mesh = m_resources.meshes().get(key.mesh);
    if (mesh == nullptr) {
      continue;
    }

    std::span<const glm::mat4> modelsSpan(models.data(), models.size());
    auto instanceUpload =
        m_scene.uploadInstances(rec, frameIndex, cursor, modelsSpan);

    if (!instanceUpload) {
      continue;
    }

    const uint32_t instanceCount = instanceUpload.instanceCount;
    PROFILE_CPU_ADD_INSTANCES(instanceCount);

    m_resources.materials().bindMaterial(cmd, m_interface.pipelineLayout(), 1,
                                         key.material);
    PROFILE_CPU_INC_DESCRIPTOR_BINDS(1);

    DrawPushConstants pushConstants{};
    pushConstants.baseInstance = instanceUpload.baseInstance;
    pushConstants.materialId = key.material;

    vkCmdPushConstants(cmd, m_interface.pipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DrawPushConstants),
                       &pushConstants);

    VkDeviceSize vertBufOffset = 0;
    VkBuffer vertBuf = mesh->vertex.handle();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertBuf, &vertBufOffset);

#if defined(ENABLE_TELEMETRY)
    const uint64_t trianglesPerInstance =
        static_cast<uint64_t>(mesh->indexCount) / 3ULL;
    const uint64_t triangles =
        trianglesPerInstance * static_cast<uint64_t>(instanceCount);
#endif

    if (mesh->indexed()) {
      VkDeviceSize indexBufOffset = 0;
      vkCmdBindIndexBuffer(cmd, mesh->index.handle(), indexBufOffset,
                           mesh->indexType);
      vkCmdDrawIndexed(cmd, mesh->indexCount, instanceCount, 0, 0, 0);
      PROFILE_CPU_INC_DRAW_CALLS(1);
      PROFILE_CPU_ADD_TRIANGLES(triangles);
    } else {
      vkCmdDraw(cmd, mesh->vertexCount, instanceCount, 0, 0);
      PROFILE_CPU_INC_DRAW_CALLS(1);
      PROFILE_CPU_ADD_TRIANGLES(triangles);
    }
  }
}

bool Renderer::drawFrame(VkPresenter &presenter, MeshHandle mesh) {
  DrawItem item{};
  item.mesh = mesh;
  item.material = UINT32_MAX;

  return drawFrame(presenter, std::span<const DrawItem>(&item, 1));
}

bool Renderer::drawFrame(VkPresenter &presenter,
                         std::span<const DrawItem> items) {
  auto endGuard = makeScopeExit([&] {
#if defined(ENABLE_TELEMETRY)
    auto *c = profiling::cpuPtr();
    auto *u = profiling::uploadPtr();

    if (c) {
      c->endInterval();
    }

    if (u) {
      u->endInterval();
    }

    if (c && u) {
      m_profileReporter.logPerFrame(c, m_gpuProfiler, u);
    }
#endif
  });

  PROFILE_CPU_SCOPE(CpuProfiler::Stat::FrameTotal);

  if (m_ctx->device() == VK_NULL_HANDLE) {
    return false;
  }

  using FrameStatus = VkFrameManager::FrameStatus;

  uint32_t imageIndex = 0;

  FrameStatus st = FrameStatus::Ok;
  st = m_frames.beginFrame(presenter.swapchain(), imageIndex, UINT64_MAX);

  if (st == FrameStatus::OutOfDate) {
    (void)recreateSwapchainDependent(presenter, m_vertPath, m_fragPath);
    return true;
  }

  if (st != FrameStatus::Ok && st != FrameStatus::Suboptimal) {
    return false;
  }

  const uint32_t frameIndex = m_frames.currentFrameIndex();

  if (!m_uploads.beginFrame(frameIndex)) {
    LOGE("Failed to begin uploads for frame {}", frameIndex);
    return false;
  }

  {
    PROFILE_CPU_SCOPE(CpuProfiler::Stat::UpdatePerFrameUBO);
    (void)m_scene.update(frameIndex, m_cameraUbo);
  }

  VkCommandBuffer cmd = m_commands.buffers()[frameIndex];
  vkResetCommandBuffer(cmd, 0);

  {
    PROFILE_CPU_SCOPE(CpuProfiler::Stat::RecordCmd);
    recordFrame(cmd, presenter, m_targets, imageIndex, items);
  }

  if (!m_uploads.flushFrame(false)) {
    LOGW("Failed to flush frame uploads");
  }

  FrameStatus sub =
      m_frames.submit(m_ctx->graphicsQueue(), imageIndex, cmd,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  if (sub != FrameStatus::Ok) {
    return false;
  }

  FrameStatus pst = m_frames.present(m_ctx->graphicsQueue(),
                                     presenter.swapchain(), imageIndex);

  m_gpuProfiler.onFrameSubmitted();
  (void)m_gpuProfiler.tryCollect(frameIndex);

  // TODO: handle SUBOPTIMAL recreate, i.e when convienent instead of now
  if (pst == FrameStatus::OutOfDate) {
    (void)recreateSwapchainDependent(presenter, m_vertPath, m_fragPath);
    return true;
  }

  return pst == FrameStatus::Ok || pst == FrameStatus::Suboptimal;
}

bool Renderer::recreateSwapchainDependent(VkPresenter &presenter,
                                          const std::string &vertSpvPath,
                                          const std::string &fragSpvPath) {
  LOGW("Recreating swapchain-dependent resources");
  profiling::EventScope scope(profiling::Event::SwapchainRecreate);

  if (m_ctx == nullptr || m_ctx->device() == VK_NULL_HANDLE) {
    return false;
  }

  VkDevice device = m_ctx->device();
  {
    profiling::EventScope w(profiling::Event::DeviceWaitIdle);
    vkDeviceWaitIdle(device);
  }

  if (!presenter.recreateSwapchain()) {
    LOGE("Swapchain recreation failed");
    return false;
  }
  m_swapLayouts.assign(presenter.imageCount(), VK_IMAGE_LAYOUT_UNDEFINED);

  if (!m_targets.recreateIfNeeded(*m_ctx, presenter)) {
    return false;
  }

  if (!m_mainPass.recreateIfNeeded(*m_ctx, presenter, m_targets, m_interface,
                                   vertSpvPath, fragSpvPath)) {
    return false;
  }

  const uint32_t imageCount = presenter.imageCount();
  LOGI("Swapchain-dependent resources recreated (images={})", imageCount);

  return m_frames.onSwapchainRecreated(imageCount);
}

MeshHandle Renderer::createMesh(const engine::Vertex *vertices,
                                uint32_t vertexCount, const uint32_t *indices,
                                uint32_t indexCount) {
  return m_resources.meshes().createMesh(m_uploads.staticRecorder(1), vertices,
                                         vertexCount, indices, indexCount);
}

MeshHandle Renderer::createMesh(const engine::MeshData &mesh) {
  return m_resources.meshes().createMesh(m_uploads.staticRecorder(1), mesh);
}

const MeshGpu *Renderer::get(MeshHandle handle) const {
  return m_resources.meshes().get(handle);
}

TextureHandle Renderer::createTextureFromFile(const std::string &path,
                                              bool flipY) {

  return m_resources.materials().createTextureFromFile(
      m_uploads.staticRecorder(2), path, flipY);
}

uint32_t Renderer::createMaterialFromTexture(TextureHandle handle) {
  // TODO: make logic for if static or frame recorder
  LOGI("Creating Material from texture");
  return m_resources.materials().createMaterialFromTexture(
      m_uploads.staticRecorder(2), handle);
}

uint32_t Renderer::createMaterialFromBaseColorFactor(const glm::vec4 &factor) {
  // TODO: make logic for if static or frame recorder
  return m_resources.materials().createMaterialFromBaseColorFactor(
      m_uploads.staticRecorder(2), factor);
}

bool Renderer::createTextureFromImage(const engine::ImageData &img,
                                      VkTexture2D &outTex) {
  return m_resources.materials().createTextureFromImage(
      m_uploads.staticRecorder(2), img, outTex);
}

void Renderer::setActiveMaterial(uint32_t materialIndex) {
  m_resources.materials().setActiveMaterial(materialIndex);
}

bool Renderer::updateMaterialGPU(uint32_t materialId, const MaterialGPU &gpu) {
  // TODO: make logic for if static or frame recorder
  return m_resources.materials().updateMaterialGPU(m_uploads.staticRecorder(3),
                                                   materialId, gpu);
}

bool Renderer::beginUpload(uint32_t frameIndex) {
  return m_uploads.beginFrame(frameIndex);
}
bool Renderer::endUpload(bool wait) { return m_uploads.flushFrame(wait); }

bool Renderer::beginStaticUploads() { return m_uploads.beginStatic(); }
bool Renderer::endStaticUploads(bool wait) {
  return m_uploads.flushStatic(wait);
}
