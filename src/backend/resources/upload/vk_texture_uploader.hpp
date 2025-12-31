#pragma once

#include "backend/frame/vk_commands.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "backend/resources/textures/vk_texture.hpp"

#include <cstdint>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class VkTextureUploader {
public:
  bool init(VmaAllocator allocator, VkDevice device, VkQueue queue,
            VkCommands *commands, UploadProfiler *profiler);
  void shutdown() noexcept;

  bool uploadRGBA8(const void *rgbaPixels, uint32_t width, uint32_t height,
                   VkTexture2D &out);

private:
  VmaAllocator m_allocator = nullptr;   // non-owning
  VkDevice m_device = VK_NULL_HANDLE;   // non-owning
  VkQueue m_queue = VK_NULL_HANDLE;     // non-owning
  VkCommands *m_commands = nullptr;     // non-owning
  UploadProfiler *m_profiler = nullptr; // non-owning

  static void cmdTransitionImage(VkCommandBuffer cmd, VkImage image,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout);
};
