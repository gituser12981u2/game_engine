#include "vulkan_context.hpp"
#include "vulkan_shader.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace {

#ifndef NDEBUG
const bool kEnableValidationLayers = true;
#else
const bool kEnableValidationLayers = false;
#endif

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset"
#endif
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  (void)messageSeverity;
  (void)messageTypes;
  (void)pUserData;

  std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n";
  return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT messenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

  if (func != nullptr) {
    func(instance, messenger, pAllocator);
  }
}

// For now, just return the extensions we know we need.
// Later you will merge this with glfwGetRequiredInstanceExtensions().
std::vector<const char *>
getRequiredExtensions(bool enableValidationLayers,
                      const std::vector<const char *> &platformExtensions) {
  std::vector<const char *> extensions;
  extensions.reserve(platformExtensions.size() + 4);

  // GLFW instance extensions
  for (const char *ext : platformExtensions) {
    extensions.push_back(ext);
  }

// MoltenVK / macOS portability.
#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

  // Debug utils for validation messages.
  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

} // namespace

VulkanContext::VulkanContext()
    : m_instance(VK_NULL_HANDLE), m_debugMessenger(VK_NULL_HANDLE),
      m_enableValidationLayers(kEnableValidationLayers) {}

VulkanContext::~VulkanContext() { shutdown(); }

bool VulkanContext::init(const std::vector<const char *> &platformExtensions) {
  if (m_enableValidationLayers && !checkValidationLayerSupport()) {
    std::cerr << "Requested validation layers not available\n";
    return false;
  }

  if (!createInstance(platformExtensions)) {
    return false;
  }

  if (m_enableValidationLayers) {
    if (!setupDebugMessenger()) {
      std::cerr << "Failed to set up debug messenger\n";
      // Non fatal
    }
  }

  if (!pickPhysicalDevice()) {
    std::cerr << "Failed to find a suitable physical device\n";
    return false;
  }

  if (!createLogicalDevice()) {
    std::cerr << "Failed to create a logical device\n";
    return false;
  }

  return true;
}

void VulkanContext::shutdown() {
  if (m_device != VK_NULL_HANDLE) {
    // Wait so nothing is in flight
    vkDeviceWaitIdle(m_device);

    // destroyFramebuffers();

    if (m_graphicsPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
      m_graphicsPipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
      m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_renderPass != VK_NULL_HANDLE) {
      vkDestroyRenderPass(m_device, m_renderPass, nullptr);
      m_renderPass = VK_NULL_HANDLE;
    }

    // destroyCommandPool();
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }

  if (m_enableValidationLayers && m_debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }

  m_physicalDevice = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_graphicsQueueFamilyIndex = UINT32_MAX;
}

bool VulkanContext::checkValidationLayerSupport() {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayer(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayer.data());

  for (const char *layerName : kValidationLayers) {
    bool found = false;
    for (const auto &layerProperties : availableLayer) {
      if (std::strcmp(layerName, layerProperties.layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::cerr << "Validation layer not found:" << layerName << "\n";
      return false;
    }
  }
  return true;
}

VulkanContext::QueueFamilyIndices
VulkanContext::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto &q = queueFamilies[i];

    if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      // TODO maybe look for compute queue or dedicated transfer
      break;
    }
  }

  return indices;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(kDeviceExtensions.begin(),
                                           kDeviceExtensions.end());

  for (const auto &ext : availableExtensions) {
    requiredExtensions.erase(ext.extensionName);
  }

  if (!requiredExtensions.empty()) {
    std::cerr << "Missing required device extensions:\n";
    for (const auto &name : requiredExtensions) {
      std::cerr << "  " << name << "\n";
    }
  }

  return requiredExtensions.empty();
}

bool VulkanContext::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    std::cerr << "No Vulkan-capable GPUs found\n";
    return false;
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

  for (VkPhysicalDevice device : devices) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    // TODO: Use scoring function to pick best graphics (i.e discrete >
    // integrated)

    if (indices.isComplete() && extensionsSupported) {
      m_physicalDevice = device;
      m_graphicsQueueFamilyIndex = indices.graphicsFamily.value();
      return true;
    }
  }

  std::cerr
      << "Failed to find a GPU with required queue families and extensions\n";
  return false;
}

bool VulkanContext::createLogicalDevice() {
  if (m_physicalDevice == VK_NULL_HANDLE) {
    std::cerr
        << "createLogicalDevice called without a selected physical device\n";
    return false;
  }

  QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
  if (!indices.isComplete()) {
    std::cerr << "Graphics queue family not found on selected device\n";
    return false;
  }

  float queuePriority = 1.0f;

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkPhysicalDeviceFeatures deviceFeatures{};
  // TODO: enable features

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(kDeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

  if (m_enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
  }

  VkResult result =
      vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDevice failed with error code: " << result << "\n";
    return false;
  }

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0,
                   &m_graphicsQueue);
  m_graphicsQueueFamilyIndex = indices.graphicsFamily.value();

  return true;
}

bool VulkanContext::createInstance(
    const std::vector<const char *> &platformExtensions) {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "3DEngine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  auto extensions =
      getRequiredExtensions(m_enableValidationLayers, platformExtensions);

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (m_enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    // Hook validation messages during instance creation
    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.pNext = nullptr;
  }

  VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateInstance failed with error code: " << result << "\n";
    return false;
  }

  return true;
}

bool VulkanContext::setupDebugMessenger() {
  if (!m_enableValidationLayers) {
    return true;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  populateDebugMessengerCreateInfo(createInfo);

  VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo,
                                                 nullptr, &m_debugMessenger);
  if (result != VK_SUCCESS) {
    std::cerr << "CreateDebugUtilsMessengerEXT failed with error code: "
              << result << "\n";
    return false;
  }

  return true;
}

bool VulkanContext::createRenderPass(VkFormat swapchainFormat) {
  if (m_device == VK_NULL_HANDLE) {
    std::cerr << "[RenderPass] Device is null\n";
    return false;
  }

  VkFormat colorFormat = swapchainFormat;
  if (colorFormat == VK_FORMAT_UNDEFINED) {
    std::cerr << "[RenderPass] Swapchain image format undefined\n";
    return false;
  }

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = colorFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult res =
      vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
  if (res != VK_SUCCESS) {
    std::cerr << "[RenderPass] vkCreateRenderPass() failed: " << res << "\n";
    m_renderPass = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[RenderPass] Render pass created\n";
  return true;
}

bool VulkanContext::createGraphicsPipeline(VkExtent2D swapchainExtent) {
  if (m_device == VK_NULL_HANDLE || m_renderPass == VK_NULL_HANDLE) {
    std::cerr << "[Pipeline] Device or render pass not initialized\n";
    return false;
  }

  // Load shaders
  VulkanShaderModule vertModule;
  VulkanShaderModule fragModule;

  if (!createShaderModuleFromFile(m_device, "shaders/bin/shader.vert.spv",
                                  vertModule)) {
    std::cerr << "[Pipeline] Failed to load vertex shader\n";
    return false;
  }

  if (!createShaderModuleFromFile(m_device, "shaders/bin/shader.frag.spv",
                                  fragModule)) {
    std::cerr << "[Pipeline] Failed to load fragment shader\n";
    return false;
  }

  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = vertModule.m_handle;
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = fragModule.m_handle;
  fragStage.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

  // Fixed function states

  // Vertex input: none for now
  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 0;
  vertexInput.pVertexBindingDescriptions = nullptr;
  vertexInput.vertexAttributeDescriptionCount = 0;
  vertexInput.pVertexAttributeDescriptions = nullptr;

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport / scissor
  VkExtent2D extent = swapchainExtent;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  // Multisampling: off for now
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.sampleShadingEnable = VK_FALSE;

  // Color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  // Pipeline layout: no descriptors/push constants for now
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  VkResult res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr,
                                        &m_pipelineLayout);
  if (res != VK_SUCCESS) {
    std::cerr << "[Pipeline] vkCreatePipelineLayout failed: " << res << "\n";
    m_pipelineLayout = VK_NULL_HANDLE;
    return false;
  }

  // Graphics pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.renderPass = m_renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &m_graphicsPipeline);
  if (res != VK_SUCCESS) {
    std::cerr << "[Pipeline] vkCreateGraphicsPipelines() failed: " << res
              << "\n";
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
    m_graphicsPipeline = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[Pipeline] Graphics pipeline created\n";
  return true;
}

bool VulkanContext::createFrameBuffers(
    const std::vector<VkImageView> &swapchainImageViews,
    VkExtent2D swapchainExtent) {
  if (m_device == VK_NULL_HANDLE || m_renderPass == VK_NULL_HANDLE) {
    std::cerr << "[Framebuffers] Device or render pass not ready\n";
    return false;
  }

  const auto &views = swapchainImageViews;

  if (views.empty()) {
    std::cerr << "[Framebuffers] No swapchain image views.\n";
    return false;
  }

  // Clean if called twice
  destroyFramebuffers();

  VkExtent2D extent = swapchainExtent;
  m_swapchainFramebuffers.resize(views.size(), VK_NULL_HANDLE);

  for (size_t i = 0; i < views.size(); ++i) {
    VkImageView attachments[] = {views[i]};

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = m_renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = attachments;
    fbInfo.width = extent.width;
    fbInfo.height = extent.height;
    fbInfo.layers = 1;

    VkResult res = vkCreateFramebuffer(m_device, &fbInfo, nullptr,
                                       &m_swapchainFramebuffers[i]);
    if (res != VK_SUCCESS) {
      std::cerr << "[Framebuffers] vkCreateFramebuffer() failed at index " << i
                << " error=" << res << "\n";
      return false;
    }
  }

  std::cout << "[Framebuffers] Created " << m_swapchainFramebuffers.size()
            << " framebuffers\n";
  return true;
}

void VulkanContext::destroyFramebuffers() {
  if (m_device == VK_NULL_HANDLE) {
    m_swapchainFramebuffers.clear();
    return;
  }

  for (VkFramebuffer fb : m_swapchainFramebuffers) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(m_device, fb, nullptr);
    }
  }
  m_swapchainFramebuffers.clear();
}

bool VulkanContext::createCommandPool() {
  if (m_device == VK_NULL_HANDLE) {
    std::cerr << "[Cmd] Device is null\n";
    return false;
  }

  if (m_graphicsQueueFamilyIndex == UINT32_MAX) {
    std::cerr << "[Cmd] Graphics queue family index invalid\n";
    return false;
  }

  if (m_commandPool != VK_NULL_HANDLE) {
    return true;
  }

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // per-frame

  VkResult res =
      vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
  if (res != VK_SUCCESS) {
    std::cerr << "[Cmd] vkCreateCommandPool failed: " << res << "\n";
    m_commandPool = VK_NULL_HANDLE;
    return false;
  }

  std::cout << "[Cmd] Command pool created\n";
  return true;
}

void VulkanContext::destroyCommandPool() {
  if (m_device == VK_NULL_HANDLE) {
    m_commandBuffers.clear();
    m_commandPool = VK_NULL_HANDLE;
    return;
  }

  if (!m_commandBuffers.empty()) {
    vkFreeCommandBuffers(m_device, m_commandPool,
                         static_cast<uint32_t>(m_commandBuffers.size()),
                         m_commandBuffers.data());
    m_commandBuffers.clear();
  }

  if (m_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }
}

bool VulkanContext::allocateCommandBuffers(uint32_t count) {
  if (m_device == VK_NULL_HANDLE || m_commandPool == VK_NULL_HANDLE) {
    std::cerr << "[Cmd] Device or command pool not read\n";
    return false;
  }

  freeCommandBuffers();

  m_commandBuffers.resize(count, VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = count;

  VkResult res =
      vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
  if (res != VK_SUCCESS) {
    std::cerr << "[Cmd] vkAllocateCommandBuffers failed: " << res << "\n";
    m_commandBuffers.clear();
    return false;
  }

  std::cout << "[Cmd] Allocated " << m_commandBuffers.size()
            << " command buffers\n";
  return true;
}

void VulkanContext::freeCommandBuffers() {
  if (m_device == VK_NULL_HANDLE || m_commandPool == VK_NULL_HANDLE) {
    m_commandBuffers.clear();
    return;
  }

  if (!m_commandBuffers.empty()) {
    vkFreeCommandBuffers(m_device, m_commandPool,
                         static_cast<uint32_t>(m_commandBuffers.size()),
                         m_commandBuffers.data());
    m_commandBuffers.clear();
  }
}
