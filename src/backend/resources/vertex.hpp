#pragma once

#include <array>
#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription bindingDescription() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  attributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attrs{};

    // Location 0 -> vec3 position
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    // Location 1 -> vec3 color
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, color);

    return attrs;
  }
};

static_assert(sizeof(Vertex) == sizeof(float) * 6);
