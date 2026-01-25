#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/gpu/descriptors/vk_material_sets.hpp"
#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/upload/vk_texture_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "engine/assets/image_data.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

struct TextureHandle {
  uint32_t id = UINT32_MAX;

  [[nodiscard]] bool valid() const noexcept { return id != UINT32_MAX; }
};

class MaterialSystem {
public:
  enum class TextureUsage : uint8_t {
    sRGB,
    UNORM,
  };

  static constexpr uint32_t kInvalidId = UINT32_MAX;

  struct MaterialDescription {
    glm::vec4 baseColorFactor{1, 1, 1, 1};
    glm::vec3 emissiveFactor{0, 0, 0};
    float metallicFactor = 0.0F;
    float roughnessFactor = 0.5F;
    float ambientOcclusionFactor = 1.0F;

    std::optional<TextureHandle> baseColorTexture;
    std::optional<TextureHandle> emissiveTexture;
    std::optional<TextureHandle> metallicRoughnessTexture;
    std::optional<TextureHandle> ambientOcclusionTexture;
    std::optional<TextureHandle> normal;

    uint32_t alphaMode = 0;
    float alphaCutoff = 0.5F;
    bool doubleSided = false;
  };

  bool init(VkBackendCtx &ctx, VkDescriptorSetLayout materialSetLayout,
            uint32_t materialCapacity, uint32_t maxTextures);
  void shutdown() noexcept;

  TextureHandle loadTextureFromFile(VkUploadContext::Recorder staticRec,
                                    const std::string &path, bool flipY,
                                    TextureUsage usage);

  bool uploadTextureFromImage(VkUploadContext::Recorder staticRec,
                              const engine::ImageData &img, TextureUsage usage,
                              VkTexture2D &outTex);

  uint32_t createMaterial(VkUploadContext::Recorder recorder,
                          const MaterialDescription &descriptor);

  bool updateMaterialGPU(VkUploadContext::Recorder recorder,
                         uint32_t materialId, const MaterialGPU &gpu);

  bool createDefaultMaterial(VkUploadContext::Recorder staticRec) noexcept;

  void bindTextureTable(VkCommandBuffer cmd, VkPipelineLayout layout,
                        uint32_t setIndex) const;

  // Material table (SSBO) is provided by SceneData
  void bindMaterialTable(VkBuffer materialTableBuffer,
                         uint32_t maxMaterialsInTable) noexcept;

  [[nodiscard]] TextureHandle defaultWhiteSrgb() const noexcept {
    return m_whiteTexture;
  }
  [[nodiscard]] TextureHandle defaultBlackSrgb() const noexcept {
    return m_blackTexture;
  }
  [[nodiscard]] TextureHandle defaultMetalRoughLinear() const noexcept {
    return m_defaultMetalRough;
  }
  [[nodiscard]] TextureHandle defaultOcclusionLinear() const noexcept {
    return m_defaultOcclusion;
  }
  [[nodiscard]] TextureHandle defaultNormalLinear() const noexcept {
    return m_defaultNormal;
  }

  [[nodiscard]] uint32_t defaultMaterial() const noexcept {
    return m_defaultMaterial;
  }

private:
  [[nodiscard]] static VkFormat formatFor(TextureUsage usage) noexcept;

  TextureHandle storeTextureAndWrite(VkTexture2D &&tex);

  [[nodiscard]] uint32_t allocMaterialId() noexcept;

  bool writeMaterialGPU(VkUploadContext::Recorder recorder, uint32_t materialId,
                        const MaterialGPU &gpu);

  VkTextureUploader m_textureUploader;

  std::vector<VkTexture2D> m_textures;
  uint32_t m_maxTextures = 0;
  VkMaterialSets m_materialSet;

  VkBuffer m_materialTable = VK_NULL_HANDLE; // non-owning
  uint32_t m_materialTableCapacity = 0;
  uint32_t m_nextMaterialId = 0;

  TextureHandle m_whiteTexture{};
  TextureHandle m_blackTexture{};
  TextureHandle m_defaultMetalRough{};
  TextureHandle m_defaultOcclusion{};
  TextureHandle m_defaultNormal{};

  uint32_t m_defaultMaterial = UINT32_MAX;
};
