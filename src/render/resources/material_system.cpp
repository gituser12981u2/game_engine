#include "render/resources/material_system.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "backend/profiling/upload_profiler.hpp"
#include "engine/assets/stb_image/stb_image_loader.hpp"
#include "render/resources/material_gpu.hpp"

#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

bool MaterialSystem::init(VkBackendCtx &ctx, VkUploadContext &upload,
                          VkDescriptorSetLayout materialSetLayout,
                          uint32_t maxMaterials, UploadProfiler *profiler) {
  shutdown();

  m_uploaderProfiler = profiler;

  VkDevice device = ctx.device();
  VmaAllocator allocator = ctx.allocator();

  if (!m_textureUploader.init(allocator, device, &upload, m_uploaderProfiler)) {
    std::cerr << "[MaterialSystem] Failed to init texture uploader\n";
    shutdown();
    return false;
  }

  if (!m_materialUploader.init(&upload, m_uploaderProfiler)) {
    std::cerr << "[MaterialSystem] Failed to init material uploader\n";
    shutdown();
    return false;
  }

  if (!m_materialSets.init(device, materialSetLayout, maxMaterials)) {
    std::cerr << "[MaterialSystem] Failed to init material sets\n";
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
  m_materialUploader.shutdown();

  m_materialTable = VK_NULL_HANDLE;
  m_materialTableCapacity = 0;

  m_defaultMaterial = UINT32_MAX;
  m_activeMaterial = UINT32_MAX;

  m_uploaderProfiler = nullptr;
}

bool MaterialSystem::createDefaultMaterial() noexcept {
  VkTexture2D tex;

  static constexpr std::array<std::uint8_t, 4> kWhiteRGBA8{255, 255, 255, 255};

  if (!m_textureUploader.uploadRGBA8(kWhiteRGBA8.data(), 1, 1, tex)) {
    std::cerr << "[MaterialSystem] Failed to create default white texture\n";
    return false;
  }

  m_textures.push_back(std::move(tex));
  m_whiteTexture = TextureHandle{static_cast<uint32_t>(m_textures.size() - 1)};

  m_defaultMaterial = createMaterialFromTexture(m_whiteTexture);
  if (m_defaultMaterial == UINT32_MAX) {
    std::cerr << "[MaterialSystem] Failed to create default material\n";
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

  const uint32_t id =
      m_materialSets.allocateForTexture(m_textures[textureHandle.id]);
  if (id == UINT32_MAX) {
    return UINT32_MAX;
  }

  MaterialGPU gpu;

  if (!writeMaterialGPU(id, gpu)) {
    std::cerr << "[MaterialSystem] Failed to write material GPU table\n";
    return UINT32_MAX;
  }

  return id;
}

uint32_t
MaterialSystem::createMaterialFromBaseColorFactor(const glm::vec4 &factor) {
  if (m_whiteTexture.id == UINT32_MAX ||
      m_whiteTexture.id >= m_textures.size() ||
      !m_textures[m_whiteTexture.id].valid()) {
    std::cerr << "[MaterialSystem] White texture not available\n";
    return UINT32_MAX;
  }

  const uint32_t id =
      m_materialSets.allocateForTexture(m_textures[m_whiteTexture.id]);
  if (id == UINT32_MAX) {
    return UINT32_MAX;
  }

  MaterialGPU gpu{};
  gpu.baseColorFactor = factor;

  if (!writeMaterialGPU(id, gpu)) {
    std::cerr << "[MaterialSystem] Failed to write material GPU table\n";
    return UINT32_MAX;
  }

  return id;
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

void MaterialSystem::bindMaterialTable(VkBuffer materialTableBuffer,
                                       uint32_t maxMaterialsInTable) noexcept {
  m_materialTable = materialTableBuffer;
  m_materialTableCapacity = maxMaterialsInTable;
}

bool MaterialSystem::writeMaterialGPU(uint32_t materialId,
                                      const MaterialGPU &gpu) {
  if (m_materialTable == VK_NULL_HANDLE) {
    std::cerr << "[MaterialSystem] Material table not bound\n";
    return false;
  }

  if (materialId == UINT32_MAX || materialId >= m_materialTableCapacity) {
    std::cerr << "[MaterialSystem] Material id out of range\n";
    return false;
  }

  const VkDeviceSize dstOffset = VkDeviceSize(materialId) * sizeof(MaterialGPU);
  return m_materialUploader.uploadOne(m_materialTable, dstOffset, gpu);
}

bool MaterialSystem::updateMaterialGPU(uint32_t materialId,
                                       const MaterialGPU &gpu) {
  return writeMaterialGPU(materialId, gpu);
}
