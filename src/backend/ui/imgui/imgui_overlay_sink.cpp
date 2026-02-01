#include "backend/ui/imgui/imgui_overlay_sink.hpp"

#include "backend/core/vk_backend_ctx.hpp"
#include "engine/logging/log.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>
#include <cstdint>

namespace ui {

static VkSampleCountFlagBits pickSampleCount1() {
  return VK_SAMPLE_COUNT_1_BIT;
}

bool ImGuiOverlaySink::createDescriptorPool(VkDevice device) noexcept {
  const std::array<VkDescriptorPoolSize, 11> poolSizes{{
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
  }};

  VkDescriptorPoolCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  info.maxSets = 1000 * static_cast<uint32_t>(poolSizes.size());
  info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  info.pPoolSizes = poolSizes.data();

  VkResult res = vkCreateDescriptorPool(device, &info, nullptr, &m_descPool);
  if (res != VK_SUCCESS) {
    LOGE("[ImGuiOverlaySink] vkCreateDescriptorPool failed");
    m_descPool = VK_NULL_HANDLE;
    return false;
  }
  return true;
}

bool ImGuiOverlaySink::init(VkBackendCtx &ctx, GlfwWindow &window,
                            uint32_t framesInFlight, VkFormat swapchainFormat) {
  shutdown(ctx.device());

  if (!window.valid()) {
    LOGE("[ImGuiOverlaySink] window invalid");
    return false;
  }
  if (framesInFlight == 0) {
    LOGE("[ImGuiOverlaySink] framesInFlight must be > 0");
    return false;
  }
  if (swapchainFormat == VK_FORMAT_UNDEFINED) {
    LOGE("[ImGuiOverlaySink] swapchainFormat invalid");
    return false;
  }

  m_framesInFlight = framesInFlight;
  m_swapchainFormat = swapchainFormat;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(window.handle(), /*install_callbacks=*/true);

  if (!createDescriptorPool(ctx.device())) {
    shutdown(ctx.device());
    return false;
  }

  VkPipelineRenderingCreateInfo pr{};
  pr.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pr.colorAttachmentCount = 1;
  pr.pColorAttachmentFormats = &m_swapchainFormat;
  pr.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
  pr.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = ctx.instance();
  initInfo.PhysicalDevice = ctx.physicalDevice();
  initInfo.Device = ctx.device();
  initInfo.QueueFamily = ctx.graphicsQueueFamily();
  initInfo.Queue = ctx.graphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = m_descPool;
  initInfo.Subpass = 0;
  initInfo.MinImageCount = framesInFlight;
  initInfo.ImageCount = framesInFlight;
  initInfo.MSAASamples = pickSampleCount1();
  initInfo.Allocator = nullptr;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.UseDynamicRendering = true;
  initInfo.PipelineRenderingCreateInfo = pr;

  ImGui_ImplVulkan_Init(&initInfo);

  ImGui_ImplVulkan_CreateFontsTexture();

  m_initialized = true;
  return true;
}

void ImGuiOverlaySink::destroyDescriptorPool(VkDevice device) noexcept {
  if (m_descPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, m_descPool, nullptr);
    m_descPool = VK_NULL_HANDLE;
  }
}

void ImGuiOverlaySink::shutdown(VkDevice device) {
  if (!m_initialized) {
    return;
  }

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  destroyDescriptorPool(device);

  m_framesInFlight = 0;
  m_swapchainFormat = VK_FORMAT_UNDEFINED;
  m_initialized = false;
}

void ImGuiOverlaySink::beginFrame(const BuildFn &buildPanels) {
  if (!m_initialized) {
    return;
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (buildPanels) {
    buildPanels();
  }

  ImGui::Render();
}

void ImGuiOverlaySink::record(VkCommandBuffer cmd, const OverlayTarget &tgt) {
  if (!m_initialized) {
    return;
  }
  if (cmd == VK_NULL_HANDLE || tgt.colorView == VK_NULL_HANDLE) {
    return;
  }

  // Dynamic rendering overlay: LOAD existing swapchain color
  VkRenderingAttachmentInfo colorAttach{};
  colorAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttach.imageView = tgt.colorView;
  colorAttach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  VkRenderingInfo ri{};
  ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  ri.renderArea = VkRect2D{.offset = {0, 0}, .extent = tgt.extent};
  ri.layerCount = 1;
  ri.colorAttachmentCount = 1;
  ri.pColorAttachments = &colorAttach;

  vkCmdBeginRendering(cmd, &ri);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  vkCmdEndRendering(cmd);
}

} // namespace ui
