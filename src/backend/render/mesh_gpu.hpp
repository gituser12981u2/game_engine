#pragma once

#include "../resources/vk_buffer.hpp"
#include <vulkan/vulkan_core.h>

struct MeshGpu {
  VkBufferObj vertex;
  VkBufferObj index;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  VkIndexType indexType = VK_INDEX_TYPE_UINT32;

  void shutdown() noexcept {
    vertex.shutdown();
    index.shutdown();
    vertexCount = 0;
    indexCount = 0;
    // TODO: make index type dynamically choose between UINT16 and UINT32,
    // and maybe UINT8
    indexType = VK_INDEX_TYPE_UINT32;
  }

  [[nodiscard]] bool indexed() const noexcept {
    return index.valid() && indexCount > 0;
  }
};
