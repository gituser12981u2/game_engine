#pragma once

#include "engine/mesh/vertex.hpp"

#include <array>
#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan_core.h>

namespace vk_vertex_layout {

inline VkVertexInputBindingDescription bindingDescription() {
  VkVertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = sizeof(engine::Vertex);
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return binding;
}

inline std::array<VkVertexInputAttributeDescription, 3>
attributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 3> attrs{};

  // Location 0 -> vec3 position
  attrs[0].location = 0;
  attrs[0].binding = 0;
  attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[0].offset = offsetof(engine::Vertex, pos);

  // Location 1 -> vec3 color
  attrs[1].location = 1;
  attrs[1].binding = 0;
  attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[1].offset = offsetof(engine::Vertex, color);

  // Location 2 -> vec2 uv
  attrs[2].location = 2;
  attrs[2].binding = 0;
  attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
  attrs[2].offset = offsetof(engine::Vertex, uv);

  return attrs;
}

} // namespace vk_vertex_layout
