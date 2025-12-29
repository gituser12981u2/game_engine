#include "backend/render/resources/material_system.hpp"

#include "engine/assets/stb_image/stb_image_loader.hpp"

#include <cstdint>
#include <iostream>

bool MaterialSystem::init(VkBackendCtx &ctx, class VkCommands &commands,
                          VkDescriptorSetLayout materialSetLayout,
                          uint32_t maxMaterials) {
  shutdown();

  VkDevice device = ctx.device();
  VmaAllocator allocator = ctx.allocator();

  if (!m_textureUploader.init(allocator, device, ctx.graphicsQueue(),
                              &commands)) {
    std::cerr << "[MaterialSystem] Failed to init texture uploader\n";
    shutdown();
    return false;
  }

  if (!m_materialSets.init(device, materialSetLayout, maxMaterials)) {
    std::cerr << "[MaterialSystem] Failed to init material sets\n";
    shutdown();
    return false;
  }

  // Create a 1x1 default white texture and material
  if (!createDefaultMaterial()) {
    std::cerr << "[MaterialSystem] Failed to create the default material";
    shutdown();
    return false;
  }

  return true;
}

void MaterialSystem::shutdown() noexcept {
  m_materialSets.shutdown();

  // Textures
  for (auto &texture : m_textures) {
    texture.shutdown();
  }
  m_textures.clear();

  m_textureUploader.shutdown();

  m_defaultMaterial = UINT32_MAX;
  m_activeMaterial = UINT32_MAX;
}

bool MaterialSystem::createDefaultMaterial() noexcept {
  VkTexture2D tex;

  static constexpr std::array<std::uint8_t, 4> kWhiteRGBA8{255, 255, 255, 255};

  if (!m_textureUploader.uploadRGBA8(kWhiteRGBA8.data(), 1, 1, tex)) {
    std::cerr << "[MaterialSystem] Failed to create default white texture\n";
    shutdown();
    return false;
  }

  m_textures.push_back(std::move(tex));
  const TextureHandle texHandle{static_cast<uint32_t>(m_textures.size() - 1)};

  m_defaultMaterial = createMaterialFromTexture(texHandle);
  if (m_defaultMaterial == UINT32_MAX) {
    std::cerr << "[MaterialSystem] Failed to create default material\n";
    shutdown();
    return false;
  }

  m_activeMaterial = m_defaultMaterial;
  return true;
}

TextureHandle MaterialSystem::createTextureFromFile(const std::string &path,
                                                    bool flipY) {
  engine::ImageData img;
  if (!engine::assets::loadImageRGBA8(path, img, flipY)) {
    std::cerr << "[MaterialSystem] Failed to load image: " << path << "\n";
    return {};
  }

  VkTexture2D tex;
  if (!m_textureUploader.uploadRGBA8(img.pixels.data(), img.width, img.height,
                                     tex)) {
    std::cerr << "[MaterialSystem] Failed to create texture from file\n";
    return {};
  }

  m_textures.push_back(std::move(tex));
  return TextureHandle{static_cast<uint32_t>(m_textures.size() - 1)};
}

bool MaterialSystem::createTextureFromImage(const engine::ImageData &img,
                                            VkTexture2D &outTex) {
  if (!img.valid()) {
    std::cerr << "[MaterialSystem] createTextureFromImage invalid image\n";
    return false;
  }

  const size_t expected = size_t(img.width) * img.height * 4ULL;
  if (img.pixels.size() != expected) {
    std::cerr << "[MaterialSystem] Image byte size mismatch: have="
              << img.pixels.size() << " expected=" << expected << "\n";
    return false;
  }

  return m_textureUploader.uploadRGBA8(img.pixels.data(), img.width, img.height,
                                       outTex);
}

uint32_t
MaterialSystem::createMaterialFromTexture(TextureHandle textureHandle) {
  if (textureHandle.id >= m_textures.size() ||
      !m_textures[textureHandle.id].valid()) {
    std::cerr << "[MaterialSystem] Invalid texture handle\n";
    return UINT32_MAX;
  }
  return m_materialSets.allocateForTexture(m_textures[textureHandle.id]);
}

void MaterialSystem::setActiveMaterial(uint32_t materialIndex) {
  m_activeMaterial = materialIndex;
}

uint32_t MaterialSystem::resolveMaterial(uint32_t overrideMaterial) const {
  // priority: override -> active -> default
  if (overrideMaterial != UINT32_MAX) {
    return overrideMaterial;
  }

  if (m_activeMaterial != UINT32_MAX) {
    return m_activeMaterial;
  }

  return m_defaultMaterial;
}

void MaterialSystem::bindMaterial(VkCommandBuffer cmd, VkPipelineLayout layout,
                                  uint32_t setIndex, uint32_t materialIndex) {
  const uint32_t mat = resolveMaterial(materialIndex);
  m_materialSets.bind(cmd, layout, setIndex, mat);
}
