#include "renderer.hpp"

#include "../../engine/camera/camera_ubo.hpp"
#include "../../engine/mesh/vertex.hpp"
#include "../presentation/vk_presenter.hpp"
#include "../resources/vk_buffer.hpp"
#include "mesh_gpu.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

bool Renderer::init(VkPhysicalDevice physicalDevice, VkDevice device,
                    VkQueue graphicsQueue, uint32_t graphicsQueueFamily,
                    VkPresenter &presenter, uint32_t framesInFlight,
                    const std::string &vertSpvPath,
                    const std::string &fragSpvPath) {
  if (device == VK_NULL_HANDLE || graphicsQueue == VK_NULL_HANDLE) {
    std::cerr << "[Renderer] Invalid device/queue\n";
    return false;
  }

  if (graphicsQueueFamily == UINT32_MAX) {
    std::cerr << "[Renderer] Invalid graphics queue family\n";
    return false;
  }

  if (framesInFlight == 0) {
    std::cerr << "[Renderer] framesInFlight must be > 0\n";
    return false;
  }

  // Re-init
  shutdown();

  m_device = device;
  m_physicalDevice = physicalDevice;
  m_graphicsQueue = graphicsQueue;
  m_graphicsQueueFamily = graphicsQueueFamily;
  m_framesInFlight = framesInFlight;

  m_vertPath = vertSpvPath;
  m_fragPath = fragSpvPath;

  if (!m_depth.init(m_physicalDevice, device, presenter.extent())) {
    std::cerr << "[Renderer] Failed to create depth buffer\n";
    shutdown();
    return false;
  }

  // Create render pass
  if (!m_renderPass.init(m_device, presenter.imageFormat(), m_depth.format())) {
    std::cerr << "[Renderer] Failed to create render pass\n";
    shutdown();
    return false;
  }

  // Create graphics pipeline
  if (!m_pipeline.init(m_device, m_renderPass.handle(), presenter.extent(),
                       m_vertPath, m_fragPath)) {
    std::cerr << "[Renderer] Failed to create graphics pipeline\n";
    shutdown();
    return false;
  }

  // Create frame buffers
  if (!m_framebuffers.init(m_device, m_renderPass.handle(),
                           presenter.imageViews(), m_depth.view(),
                           presenter.extent())) {
    std::cerr << "[Renderer] Failed to create framebuffers\n";
    shutdown();
    return false;
  }

  // Create command pool
  if (!m_commands.init(m_device, m_graphicsQueueFamily)) {
    std::cerr << "[Renderer] Failed to create the command pool\n";
    shutdown();
    return false;
  }

  // Create camera
  if (!m_camera.init(m_physicalDevice, m_device,
                     m_pipeline.descriptorSetLayout(), m_framesInFlight,
                     sizeof(CameraUBO))) {
    std::cerr << "[Renderer] Failed to init camera UBO\n";
    shutdown();
    return false;
  }

  if (!m_uploader.init(m_physicalDevice, m_device, m_graphicsQueue,
                       &m_commands)) {
    std::cerr << "[Renderer] Failed to init uploader\n";
    shutdown();
    return false;
  }

  // Allocate buffer per frame
  if (!m_commands.allocate(m_framesInFlight)) {
    std::cerr << "[Renderer] Failed to allocate command buffers\n";
    shutdown();
    return false;
  }

  const uint32_t imageCount =
      static_cast<uint32_t>(presenter.imageViews().size());

  if (!m_frames.init(m_device, m_framesInFlight, imageCount)) {
    std::cerr << "[Renderer] Failed to create frame sync objects\n";
    shutdown();
    return false;
  }
  return true;
}

// Destroy in reverse initialization order
void Renderer::shutdown() noexcept {
  if (m_device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(m_device);
  }

  // Commands-dependents
  m_frames.shutdown();

  for (auto &mesh : m_meshes) {
    mesh.shutdown();
  }
  m_meshes.clear();

  m_uploader.shutdown();
  m_camera.shutdown();

  m_commands.shutdown();

  // Swapchain-dependents
  m_framebuffers.shutdown();
  m_pipeline.shutdown();
  m_renderPass.shutdown();
  m_depth.shutdown();

  m_device = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_graphicsQueueFamily = UINT32_MAX;
  m_framesInFlight = 0;

  m_vertPath.clear();
  m_fragPath.clear();
}

MeshHandle Renderer::createMesh(const engine::Vertex *vertices,
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
    std::cerr << "[Renderer] vertex upload failed\n";
    return {};
  }

  gpu.vertexCount = vertexCount;

  if (indices != nullptr && indexCount > 0) {
    const VkDeviceSize ibSize = VkDeviceSize(sizeof(uint32_t)) * indexCount;
    if (!m_uploader.uploadToDeviceLocalBuffer(
            indices, ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, gpu.index)) {
      std::cerr << "[Renderer] indice upload failed\n";
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

const MeshGpu *Renderer::mesh(MeshHandle handle) const {
  if (handle.id >= m_meshes.size()) {
    return nullptr;
  }

  return &m_meshes[handle.id];
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkFramebuffer fb,
                           VkExtent2D extent, const MeshGpu &mesh) {
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &beginInfo);

  std::array<VkClearValue, 2> clears{};
  clears[0].color = {{0.05F, 0.05F, 0.08F, 1.0F}};
  clears[1].depthStencil =
      VkClearDepthStencilValue{.depth = 1.0F, .stencil = 0};

  VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBegin.renderPass = m_renderPass.handle();
  rpBegin.framebuffer = fb;
  rpBegin.renderArea.offset = VkOffset2D{0, 0};
  rpBegin.renderArea.extent = extent;
  rpBegin.clearValueCount = static_cast<uint32_t>(clears.size());
  rpBegin.pClearValues = clears.data();

  vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipeline.pipeline());

  m_camera.bind(cmd, m_pipeline.layout(), 0, m_frames.currentFrameIndex());

  // Push constant (model matrix)
  glm::mat4 model = glm::mat4(1.0F);

  // TODO REMOVE
  model = glm::rotate(model, m_timeSeconds, glm::vec3(0.0F, 0.0F, 1.0F));

  vkCmdPushConstants(cmd, m_pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(glm::mat4), &model);

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

  VkDeviceSize vertBufOffset = 0;
  VkDeviceSize indexBufOffset = 0;
  VkBuffer vertBuf = mesh.vertex.handle();
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertBuf, &vertBufOffset);

  if (mesh.indexed()) {
    vkCmdBindIndexBuffer(cmd, mesh.index.handle(), indexBufOffset,
                         mesh.indexType);
    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
  } else {
    vkCmdDraw(cmd, mesh.vertexCount, 1, 0, 0);
  }

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
}

bool Renderer::drawFrame(VkPresenter &presenter, MeshHandle mesh) {
  if (m_device == VK_NULL_HANDLE) {
    return false;
  }

  // TODO REMOVE
  static auto start = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  std::chrono::duration<float> dt = now - start;
  m_timeSeconds = dt.count();

  using FrameStatus = VkFrameManager::FrameStatus;

  uint32_t imageIndex = 0;
  auto st = m_frames.beginFrame(presenter.swapchain(), imageIndex);
  if (st == FrameStatus::OutOfDate) {
    (void)recreateSwapchainDependent(presenter, m_vertPath, m_fragPath);
    return true;
  }

  if (st != FrameStatus::Ok && st != FrameStatus::Suboptimal) {
    return false;
  }

  const uint32_t frameIndex = m_frames.currentFrameIndex();

  if (!m_camera.update(frameIndex, &m_cameraUbo, sizeof(m_cameraUbo))) {
    std::cerr << "[Renderer] Failed to update camera UBO\n";
  }

  VkCommandBuffer cmd = m_commands.buffers()[frameIndex];
  vkResetCommandBuffer(cmd, 0);

  recordFrame(cmd, m_framebuffers.at(imageIndex), presenter.extent(),
              m_meshes[mesh.id]);

  auto pst = m_frames.submitAndPresent(m_graphicsQueue, presenter.swapchain(),
                                       imageIndex, cmd);

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
  if (m_device == VK_NULL_HANDLE) {
    return false;
  }

  const VkFormat oldFormat = presenter.imageFormat();
  const VkExtent2D oldExtent = presenter.extent();

  vkDeviceWaitIdle(m_device);

  m_framebuffers.shutdown();

  if (!presenter.recreateSwapchain()) {
    return false;
  }

  // Keep pipeline and render pass unless format changes
  const VkFormat newFormat = presenter.imageFormat();
  const bool formatChanged = (newFormat != oldFormat);

  // Keep depth unless extent changes
  const VkExtent2D newExtent = presenter.extent();
  const bool extentChanged = oldExtent.width != newExtent.width ||
                             oldExtent.height != newExtent.height;

  if (extentChanged) {
    m_depth.shutdown();

    // Recreate depth
    if (!m_depth.init(m_physicalDevice, m_device, presenter.extent())) {
      return false;
    }
  }

  if (formatChanged) {
    m_pipeline.shutdown();
    m_renderPass.shutdown();

    if (!m_renderPass.init(m_device, presenter.imageFormat(),
                           m_depth.format())) {
      return false;
    }

    if (!m_pipeline.init(m_device, m_renderPass.handle(), presenter.extent(),
                         vertSpvPath, fragSpvPath)) {
      return false;
    }
  }

  // Recreate framebuffers
  if (!m_framebuffers.init(m_device, m_renderPass.handle(),
                           presenter.imageViews(), m_depth.view(),
                           presenter.extent())) {
    return false;
  }

  const uint32_t imageCount = presenter.imageCount();
  return m_frames.onSwapchainRecreated(imageCount);
}
