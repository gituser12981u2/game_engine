#include "renderer.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/profiling/cpu_profiler.hpp"
#include "backend/profiling/profiling_logger.hpp"
#include "backend/profiling/vk_gpu_profiler.hpp"
#include "backend/render/resources/mesh_gpu.hpp"
#include "backend/util/scope_exit.hpp"
#include "engine/geometry/transform.hpp"
#include "engine/mesh/mesh_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

bool Renderer::init(VkBackendCtx &ctx, VkPresenter &presenter,
                    uint32_t framesInFlight, const std::string &vertSpvPath,
                    const std::string &fragSpvPath) {
  if (ctx.device() == VK_NULL_HANDLE ||
      ctx.physicalDevice() == VK_NULL_HANDLE ||
      ctx.graphicsQueue() == VK_NULL_HANDLE ||
      ctx.graphicsQueueFamily() == UINT32_MAX) {
    std::cerr << "[Renderer] backend ctx not initialized\n";
    return false;
  }

  if (framesInFlight == 0) {
    std::cerr << "[Renderer] framesInFlight must be > 0\n";
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_framesInFlight = framesInFlight;
  m_vertPath = vertSpvPath;
  m_fragPath = fragSpvPath;

  VkDevice device = m_ctx->device();

  if (!m_gpuProfiler.init(*m_ctx, m_framesInFlight)) {
    std::cerr << "[Renderer] Failed to init GPU profiler\n";
    shutdown();
    return false;
  }

  // Create swapchain targets
  if (!m_targets.init(*m_ctx, presenter)) {
    std::cerr << "[Renderer] Failed to create depth targets\n";
    shutdown();
    return false;
  }

  // Create shader interface
  if (!m_interface.init(device)) {
    std::cerr << "[Renderer] Failed to create shader interface\n";
    shutdown();
    return false;
  }

  // Create main pass
  if (!m_mainPass.init(*m_ctx, presenter, m_targets, m_interface, m_vertPath,
                       m_fragPath)) {
    std::cerr << "[Renderer] Failed to init main pass\n";
    shutdown();
    return false;
  }

  // Create frame buffers
  if (!m_fbos.init(*m_ctx, presenter, m_targets, m_mainPass)) {
    std::cerr << "[Renderer] Failed to create framebuffers\n";
    shutdown();
    return false;
  }

  // Create command pool
  if (!m_commands.init(*m_ctx)) {
    std::cerr << "[Renderer] Failed to create the command pool\n";
    shutdown();
    return false;
  }

  if (!m_perFrame.init(*m_ctx, m_framesInFlight, m_interface)) {
    std::cerr << "[Renderer] Failed to init per-frame data\n";
    shutdown();
    return false;
  }

  if (!m_resources.init(*m_ctx, m_commands, m_interface, &m_uploadProfiler)) {
    std::cerr << "[Renderer] Failed to init resource store\n";
    shutdown();
    return false;
  }

  // Allocate buffer per frame
  if (!m_commands.allocate(m_framesInFlight)) {
    std::cerr << "[Renderer] Failed to allocate command buffers\n";
    shutdown();
    return false;
  }

  if (!m_frames.init(device, m_framesInFlight, presenter.imageCount())) {
    std::cerr << "[Renderer] Failed to create frame sync objects\n";
    shutdown();
    return false;
  }

  // m_uploadProfiler.endFrame();
  // profiling::logUploadOnce("ResourceStore init", m_uploadProfiler);

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
  m_commands.shutdown();

  m_perFrame.shutdown();

  // Swapchain-dependents
  m_fbos.shutdown();
  m_mainPass.shutdown();
  m_interface.shutdown();
  m_targets.shutdown();

  m_ctx = nullptr;
  m_framesInFlight = 0;
  m_gpuProfiler.shutdown();

  m_vertPath.clear();
  m_fragPath.clear();
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkFramebuffer fb,
                           VkExtent2D extent, const MeshHandle mesh,
                           uint32_t material, glm::vec3 pos, glm::vec3 rotRad,
                           glm::vec3 scale) {
  DrawItem item{};
  item.mesh = mesh;
  item.material = material;
  item.model = engine::makeModel(pos, rotRad, scale);

  recordFrame(cmd, fb, extent, std::span<const DrawItem>(&item, 1));
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkFramebuffer fb,
                           VkExtent2D extent, std::span<const DrawItem> items) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cmd, &beginInfo);

  const uint32_t frameIndex = m_frames.currentFrameIndex();

  m_gpuProfiler.beginFrameCmd(cmd, frameIndex);
  m_gpuProfiler.markFrameBegin(cmd, frameIndex);

  std::array<VkClearValue, 2> clears{};
  clears[0].color = {{0.05F, 0.05F, 0.08F, 1.0F}};
  clears[1].depthStencil =
      VkClearDepthStencilValue{.depth = 1.0F, .stencil = 0};

  VkRenderPassBeginInfo rpBegin{};
  rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpBegin.renderPass = m_mainPass.renderPass();
  rpBegin.framebuffer = fb;
  rpBegin.renderArea.offset = VkOffset2D{0, 0};
  rpBegin.renderArea.extent = extent;
  rpBegin.clearValueCount = static_cast<uint32_t>(clears.size());
  rpBegin.pClearValues = clears.data();

  m_gpuProfiler.markMainPassBegin(cmd, frameIndex);
  vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_mainPass.pipeline());
  m_cpuProfiler.incPipelineBinds(1);

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

  m_perFrame.bind(cmd, m_interface, m_frames.currentFrameIndex());
  m_cpuProfiler.incDescriptorBinds(1);

  for (const DrawItem &item : items) {
    m_cpuProfiler.incDrawCalls(1);
    m_cpuProfiler.incDescriptorBinds(1);

    const MeshGpu *mesh = m_resources.meshes().get(item.mesh);
    if (mesh == nullptr) {
      continue;
    }

    m_resources.materials().bindMaterial(cmd, m_interface.pipelineLayout(), 1,
                                         item.material);

    vkCmdPushConstants(cmd, m_interface.pipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                       &item.model);

    VkDeviceSize vertBufOffset = 0;
    VkBuffer vertBuf = mesh->vertex.handle();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertBuf, &vertBufOffset);

    if (mesh->indexed()) {
      VkDeviceSize indexBufOffset = 0;
      vkCmdBindIndexBuffer(cmd, mesh->index.handle(), indexBufOffset,
                           mesh->indexType);
      vkCmdDrawIndexed(cmd, mesh->indexCount, 1, 0, 0, 0);
      m_cpuProfiler.addTriangles(mesh->indexCount / 3);
    } else {
      vkCmdDraw(cmd, mesh->vertexCount, 1, 0, 0);
      m_cpuProfiler.addTriangles(mesh->vertexCount / 3);
    }
  }

  vkCmdEndRenderPass(cmd);
  m_gpuProfiler.markMainPassEnd(cmd, frameIndex);

  m_gpuProfiler.markFrameEnd(cmd, frameIndex);
  vkEndCommandBuffer(cmd);
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
    m_cpuProfiler.endFrame();
    m_uploadProfiler.endFrame();
    m_profileReporter.logPerFrame(m_cpuProfiler, m_gpuProfiler,
                                  m_uploadProfiler);
  });

  CpuProfiler::Scope frameScope(m_cpuProfiler, CpuProfiler::Stat::FrameTotal);

  if (m_ctx->device() == VK_NULL_HANDLE) {
    return false;
  }

  using FrameStatus = VkFrameManager::FrameStatus;

  uint32_t imageIndex = 0;

  FrameStatus st = FrameStatus::Ok;
  st = m_frames.beginFrame(presenter.swapchain(), imageIndex, UINT64_MAX,
                           &m_cpuProfiler);

  if (st == FrameStatus::OutOfDate) {
    (void)recreateSwapchainDependent(presenter, m_vertPath, m_fragPath);
    return true;
  }

  if (st != FrameStatus::Ok && st != FrameStatus::Suboptimal) {
    return false;
  }

  const uint32_t frameIndex = m_frames.currentFrameIndex();

  {
    CpuProfiler::Scope s(m_cpuProfiler, CpuProfiler::Stat::UpdatePerFrameUBO);
    (void)m_perFrame.update(frameIndex, m_cameraUbo);
  }

  VkCommandBuffer cmd = m_commands.buffers()[frameIndex];
  vkResetCommandBuffer(cmd, 0);

  {
    CpuProfiler::Scope s(m_cpuProfiler, CpuProfiler::Stat::RecordCmd);
    recordFrame(cmd, m_fbos.at(imageIndex), presenter.swapchainExtent(), items);
  }

  FrameStatus sub = m_frames.submit(
      m_ctx->graphicsQueue(), imageIndex, cmd,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, &m_cpuProfiler);
  if (sub != FrameStatus::Ok) {
    return false;
  }

  FrameStatus pst =
      m_frames.present(m_ctx->graphicsQueue(), presenter.swapchain(),
                       imageIndex, &m_cpuProfiler);

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
  profiling::EventScope scope(profiling::Event::SwapchainRecreate);

  if (m_ctx == nullptr || m_ctx->device() == VK_NULL_HANDLE) {
    return false;
  }

  VkDevice device = m_ctx->device();
  {
    profiling::EventScope w(profiling::Event::DeviceWaitIdle);
    vkDeviceWaitIdle(device);
  }

  m_fbos.shutdown();

  if (!presenter.recreateSwapchain()) {
    return false;
  }

  if (!m_targets.recreateIfNeeded(*m_ctx, presenter)) {
    return false;
  }

  if (!m_mainPass.recreateIfNeeded(*m_ctx, presenter, m_targets, m_interface,
                                   vertSpvPath, fragSpvPath)) {
    return false;
  }

  // Recreate framebuffers
  if (!m_fbos.rebuild(*m_ctx, presenter, m_targets, m_mainPass)) {
    return false;
  }

  const uint32_t imageCount = presenter.imageCount();
  return m_frames.onSwapchainRecreated(imageCount);
}

MeshHandle Renderer::createMesh(const engine::Vertex *vertices,
                                uint32_t vertexCount, const uint32_t *indices,
                                uint32_t indexCount) {
  return m_resources.meshes().createMesh(vertices, vertexCount, indices,
                                         indexCount);
}

MeshHandle Renderer::createMesh(const engine::MeshData &mesh) {
  return m_resources.meshes().createMesh(mesh);
}

const MeshGpu *Renderer::get(MeshHandle handle) const {
  return m_resources.meshes().get(handle);
}

TextureHandle Renderer::createTextureFromFile(const std::string &path,
                                              bool flipY) {
  return m_resources.materials().createTextureFromFile(path, flipY);
}

uint32_t Renderer::createMaterialFromTexture(TextureHandle handle) {
  return m_resources.materials().createMaterialFromTexture(handle);
}

bool Renderer::createTextureFromImage(const engine::ImageData &img,
                                      VkTexture2D &outTex) {
  return m_resources.materials().createTextureFromImage(img, outTex);
}

void Renderer::setActiveMaterial(uint32_t materialIndex) {
  m_resources.materials().setActiveMaterial(materialIndex);
}
