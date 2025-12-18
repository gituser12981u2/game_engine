#pragma once

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

struct Vertex {
  float pos[3];
  float color[3];

  static VkVertexInputBindingDescription bindingDescription() {
    VkVertexInputBindingDescription b{};
    b.binding = 0;
    b.stride = sizeof(Vertex);
    b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return b;
  }

  static void attributeDescriptions(VkVertexInputAttributeDescription out[2]) {
    // Location 0 -> vec3 position
    out[0].location = 0;
    out[0].binding = 0;
    out[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    out[0].offset = static_cast<uint32_t>(offsetof(Vertex, pos));

    // Location 1 -> vec3 color
    out[1].location = 1;
    out[1].binding = 0;
    out[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    out[1].offset = static_cast<uint32_t>(offsetof(Vertex, color));
  }
};
static_assert(sizeof(Vertex) == sizeof(float) * 6);
