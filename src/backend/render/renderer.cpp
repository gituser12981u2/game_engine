#include "renderer.hpp"
#include "../presentation/vk_presenter.hpp"
#include "../resources/vk_buffer.hpp"
#include "camera_ubo.hpp"
#include "vertex.hpp"

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

  // Create render pass
  if (!m_renderPass.init(m_device, presenter.imageFormat())) {
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
                           presenter.imageViews(), presenter.extent())) {
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

  // Create Geometry
  if (!initTestGeometry()) {
    std::cerr << "[Renderer] Failed to init geometry\n";
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
  // Commands-dependents
  m_frames.shutdown();
  m_mesh.shutdown();
  m_uploader.shutdown();
  m_camera.shutdown();

  m_commands.shutdown();

  // Swapchain-dependents
  m_framebuffers.shutdown();
  m_pipeline.shutdown();
  m_renderPass.shutdown();

  m_device = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_graphicsQueueFamily = UINT32_MAX;
  m_framesInFlight = 0;

  m_vertPath.clear();
  m_fragPath.clear();
}

bool Renderer::initTestGeometry() {
  // Triangle
  // const std::array<Vertex, 3> verts = {{
  //     {{0.0f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}}, // yellow (bottom)
  //     {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}},  // magenta (top right)
  //     {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}}, // cyan (top left)
  // }};

  // Square
  const std::array<Vertex, 4> verts = {{
      {glm::vec3(-0.5F, -0.5F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F)},
      {glm::vec3(0.5F, -0.5F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)},
      {glm::vec3(0.5F, 0.5F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F)},
      {glm::vec3(-0.5F, 0.5F, 0.0F), glm::vec3(1.0F, 1.0F, 0.0F)},
  }};

  const std::array<uint32_t, 6> indices = {{0, 1, 2, 2, 3, 0}};

  // Circle
  // constexpr uint32_t SEGMENTS = 32;
  // std::vector<Vertex> verts;
  // std::vector<uint32_t> indices;
  //
  // verts.push_back({{0.0F, 0.0F, 0.0F}, {1, 1, 1}});
  //
  // for (uint32_t i = 0; i <= SEGMENTS; ++i) {
  //   float t = (float)i / SEGMENTS;
  //   float angle = t * 2.0F * std::numbers::pi_v<float>;
  //
  //   float x = std::cos(angle) * 0.5F;
  //   float y = std::sin(angle) * 0.5F;
  //
  //   verts.push_back({{x, y, 0.0F}, {1, 0, 0}});
  // }
  //
  // for (uint32_t i = 1; i <= SEGMENTS; ++i) {
  //   indices.push_back(0);
  //   indices.push_back(i);
  //   indices.push_back(i + 1);
  // }
  //
  // poincare disk in R^2
  // const std::array<Vertex, 8> verts = {{
  //     // Bottom arc (curving inward)
  //     {{-0.6f, -0.4f, 0.0f}, {1, 0, 0}},
  //     {{0.6f, -0.4f, 0.0f}, {1, 0, 0}},
  //
  //     // Right arc
  //     {{0.8f, -0.1f, 0.0f}, {0, 1, 0}},
  //     {{0.8f, 0.1f, 0.0f}, {0, 1, 0}},
  //
  //     // Top arc
  //     {{0.6f, 0.4f, 0.0f}, {0, 0, 1}},
  //     {{-0.6f, 0.4f, 0.0f}, {0, 0, 1}},
  //
  //     // Left arc
  //     {{-0.8f, 0.1f, 0.0f}, {1, 1, 0}},
  //     {{-0.8f, -0.1f, 0.0f}, {1, 1, 0}},
  // }};
  //
  // const std::array<uint32_t, 18> indices = {
  //     {0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 0, 0, 2, 4, 4, 6, 0}};
  //
  m_mesh.shutdown();

  if (!m_uploader.uploadToDeviceLocalBuffer(
          verts.data(), sizeof(Vertex) * verts.size(),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_mesh.vertex)) {
    return false;
  }

  if (!m_uploader.uploadToDeviceLocalBuffer(
          indices.data(), sizeof(uint32_t) * indices.size(),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_mesh.index)) {
    return false;
  }

  m_mesh.vertexCount = static_cast<uint32_t>(verts.size());
  m_mesh.indexCount = static_cast<uint32_t>(indices.size());
  m_mesh.indexType = VK_INDEX_TYPE_UINT32;

  return true;
}

void Renderer::recordFrame(VkCommandBuffer cmd, VkFramebuffer fb,
                           VkExtent2D extent) {
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkClearValue clear{};
  clear.color = {{0.05F, 0.05F, 0.08F, 1.0F}};

  VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBegin.renderPass = m_renderPass.handle();
  rpBegin.framebuffer = fb;
  rpBegin.renderArea.offset = VkOffset2D{0, 0};
  rpBegin.renderArea.extent = extent;
  rpBegin.clearValueCount = 1;
  rpBegin.pClearValues = &clear;

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
  VkBuffer vertBuf = m_mesh.vertex.handle();
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertBuf, &vertBufOffset);

  // TODO: use m_mesh.indexed to conditionally vkCmdDraw if not indexed
  // vkCmdDraw(cmd, m_vertexCount, 1, 0, 0);

  VkDeviceSize indexBufOffset = 0;
  vkCmdBindIndexBuffer(cmd, m_mesh.index.handle(), indexBufOffset,
                       m_mesh.indexType);
  vkCmdDrawIndexed(cmd, m_mesh.indexCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
}

bool Renderer::drawFrame(VkPresenter &presenter) {
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

  // Camera
  // TODO: move this to its own class and make a controller too
  CameraUBO ubo{};
  ubo.view =
      glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F),
                  glm::vec3(0.0F, 0.0F, 1.0F));

  const auto extent = presenter.extent();
  const float width = static_cast<float>(extent.width);
  const float height =
      static_cast<float>(extent.height > 0 ? extent.height : 1U);
  const float aspect = width / height;

  ubo.proj = glm::perspective(glm::radians(60.0F), aspect, 0.1F, 100.0F);
  ubo.proj[1][1] *= -1.0F; // Vulkan clip space

  if (!m_camera.update(frameIndex, &ubo, sizeof(ubo))) {
    std::cerr << "[Renderer] Failed to update camera UBO\n";
  }

  VkCommandBuffer cmd = m_commands.buffers()[frameIndex];
  vkResetCommandBuffer(cmd, 0);

  recordFrame(cmd, m_framebuffers.at(imageIndex), presenter.extent());

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

  vkDeviceWaitIdle(m_device);

  m_framebuffers.shutdown();

  if (!presenter.recreateSwapchain()) {
    return false;
  }

  // Keep pipeline and render pass unless format changes
  const VkFormat newFormat = presenter.imageFormat();
  const bool formatChanged = (newFormat != oldFormat);

  if (formatChanged) {
    m_pipeline.shutdown();
    m_renderPass.shutdown();

    if (!m_renderPass.init(m_device, presenter.imageFormat())) {
      return false;
    }

    if (!m_pipeline.init(m_device, m_renderPass.handle(), presenter.extent(),
                         vertSpvPath, fragSpvPath)) {
      return false;
    }
  }

  // Recreate framebuffers
  if (!m_framebuffers.init(m_device, m_renderPass.handle(),
                           presenter.imageViews(), presenter.extent())) {
    return false;
  }

  const uint32_t imageCount = presenter.imageCount();
  return m_frames.onSwapchainRecreated(imageCount);
}
