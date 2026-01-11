#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_material_sets.hpp"
#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/upload/vk_material_uploader.hpp"
#include "backend/gpu/upload/vk_texture_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "engine/assets/image_data.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan_core.h>

class UploadProfiler;

struct TextureHandle {
  uint32_t id = UINT32_MAX;
};

class MaterialSystem {
public:
  bool init(VkBackendCtx &ctx, VkDescriptorSetLayout materialSetLayout,
            uint32_t materialCapacity, UploadProfiler *profiler = nullptr);
  void shutdown() noexcept;

  TextureHandle createTextureFromFile(VkUploadContext::Recorder staticRec,
                                      const std::string &path, bool flipY);
  bool createTextureFromImage(VkUploadContext::Recorder staticRec,
                              const engine::ImageData &img,
                              VkTexture2D &outTex);

  uint32_t createMaterialFromTexture(VkUploadContext::Recorder recorder,
                                     TextureHandle textureHandle);
  uint32_t createMaterialFromBaseColorFactor(VkUploadContext::Recorder recorder,
                                             const glm::vec4 &factor);

  void setActiveMaterial(uint32_t materialIndex);
  [[nodiscard]] uint32_t setActiveMaterial() const { return m_activeMaterial; }

  void bindMaterial(VkCommandBuffer cmd, VkPipelineLayout layout,
                    uint32_t setIndex, uint32_t materialIndex);

  void bindMaterialTable(VkBuffer materialTableBuffer,
                         uint32_t maxMaterialsInTable) noexcept;

  bool updateMaterialGPU(VkUploadContext::Recorder recorder,
                         uint32_t materialId, const MaterialGPU &gpu);

  bool createDefaultMaterial(VkUploadContext::Recorder staticRec) noexcept;

  [[nodiscard]] uint32_t resolveMaterial(uint32_t overrideMaterial) const;

private:
  bool writeMaterialGPU(VkUploadContext::Recorder recorder, uint32_t materialId,
                        const MaterialGPU &gpu);

  VkTextureUploader m_textureUploader;
  VkMaterialUploader m_materialUploader;

  std::vector<VkTexture2D> m_textures;
  VkMaterialSets m_materialSets;

  VkBuffer m_materialTable = VK_NULL_HANDLE; // non-owning
  uint32_t m_materialTableCapacity = 0;

  uint32_t m_defaultMaterial = UINT32_MAX;
  TextureHandle m_whiteTexture{UINT32_MAX};

  uint32_t m_activeMaterial = UINT32_MAX;

  UploadProfiler *m_uploaderProfiler = nullptr; // non-owning
};
