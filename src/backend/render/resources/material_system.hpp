#pragma once

#include "backend/core/vk_backend_ctx.hpp"
#include "backend/frame/vk_commands.hpp"
#include "backend/resources/descriptors/vk_material_sets.hpp"
#include "backend/resources/textures/vk_texture.hpp"
#include "backend/resources/upload/vk_texture_uploader.hpp"
#include "engine/assets/image_data.hpp"

#include <cstdint>

struct TextureHandle {
  uint32_t id = UINT32_MAX;
};

class MaterialSystem {
public:
  // TODO: make maxMaterials dynamic
  bool init(VkBackendCtx &ctx, class VkCommands &commands,
            VkDescriptorSetLayout materialSetLayout,
            uint32_t maxMaterials = 128);
  void shutdown() noexcept;

  TextureHandle createTextureFromFile(const std::string &path, bool flipY);
  bool createTextureFromImage(const engine::ImageData &img,
                              VkTexture2D &outTex);

  uint32_t createMaterialFromTexture(TextureHandle textureHandle);

  void setActiveMaterial(uint32_t materialIndex);
  [[nodiscard]] uint32_t setActiveMaterial() const { return m_activeMaterial; }

  void bindMaterial(VkCommandBuffer cmd, VkPipelineLayout layout,
                    uint32_t setIndex, uint32_t materialIndex);

  [[nodiscard]] uint32_t resolveMaterial(uint32_t overrideMaterial) const;

  bool rebind(VkBackendCtx &ctx, VkCommands &commands) {
    return m_textureUploader.init(ctx.allocator(), ctx.device(),
                                  ctx.graphicsQueue(), &commands);
  }

private:
  bool createDefaultMaterial() noexcept;

  VkTextureUploader m_textureUploader;
  std::vector<VkTexture2D> m_textures;

  VkMaterialSets m_materialSets;
  uint32_t m_defaultMaterial = UINT32_MAX;
  uint32_t m_activeMaterial = UINT32_MAX;
};
