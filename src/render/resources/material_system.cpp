#include "render/resources/material_system.hpp"

#include "backend/gpu/textures/vk_texture.hpp"
#include "backend/gpu/upload/vk_material_uploader.hpp"
#include "backend/gpu/upload/vk_upload_context.hpp"
#include "engine/assets/stb_image/stb_image_loader.hpp"
#include "engine/logging/log.hpp"
#include "render/resources/material_gpu.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace {

uint32_t clampMaterialSsboCapactiy(VkPhysicalDevice physicalDevice,
                                   uint32_t requested) noexcept {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(physicalDevice, &props);

  const VkDeviceSize maxBytes = props.limits.maxStorageBufferRange;
  const uint32_t maxElements = maxBytes >= sizeof(MaterialGPU)
                                   ? uint32_t(maxBytes / sizeof(MaterialGPU))
                                   : 0U;

  if (maxElements == 0) {
    return 0;
  }

  return std::min(requested, maxElements);
}

} // namespace

bool MaterialSystem::init(VkBackendCtx &ctx,
                          VkDescriptorSetLayout materialSetLayout,
                          uint32_t materialCapacity, uint32_t maxTextures) {
  shutdown();

  if (!m_textureUploader.init(ctx)) {
    LOGE("Texture uploader initialization failed");
    shutdown();
    return false;
  }

  const uint32_t cappedCapacity =
      clampMaterialSsboCapactiy(ctx.physicalDevice(), materialCapacity);
  if (cappedCapacity == 0) {
    LOGE("Material SSBO capacity invalid after clamp");
    shutdown();
    return false;
  }

  if (!m_materialSet.init(ctx.device(), materialSetLayout, maxTextures)) {
    LOGE("Material sets initialization failed");
    shutdown();
    return false;
  }

  m_nextMaterialId = 0;
  m_maxTextures = maxTextures;
  m_defaultMaterial = UINT32_MAX;

  return true;
}

void MaterialSystem::shutdown() noexcept {
  m_materialSet.shutdown();

  // Textures
  for (auto &texture : m_textures) {
    texture.shutdown();
  }
  m_textures.clear();
  m_maxTextures = 0;

  m_textureUploader.shutdown();

  m_materialTable = VK_NULL_HANDLE;
  m_materialTableCapacity = 0;
  m_nextMaterialId = 0;

  m_whiteTexture = {};
  m_blackTexture = {};
  m_defaultMetalRough = {};
  m_defaultOcclusion = {};
  m_defaultNormal = {};

  m_defaultMaterial = UINT32_MAX;
}

uint32_t MaterialSystem::allocMaterialId() noexcept {
  if (m_materialTable == VK_NULL_HANDLE || m_materialTableCapacity == 0) {
    LOGE("MaterialId allocation failed because of improper material table "
         "setup");
    return UINT32_MAX;
  }

  const uint32_t id = m_nextMaterialId++;
  if (id >= m_materialTableCapacity) {
    LOGE("MaterialId is larger than material table capacity");
    return UINT32_MAX;
  }

  return id;
}

VkFormat MaterialSystem::formatFor(TextureUsage usage) noexcept {
  switch (usage) {
  case TextureUsage::sRGB:
    return VK_FORMAT_R8G8B8A8_SRGB;
  default:
    return VK_FORMAT_R8G8B8A8_UNORM;
  }
}

TextureHandle MaterialSystem::storeTextureAndWrite(VkTexture2D &&tex) {
  if (!tex.valid()) {
    LOGE("Texture is invalid");
    return {};
  }

  const uint32_t slot = static_cast<uint32_t>(m_textures.size());
  if (slot >= m_maxTextures) {
    LOGE("Texture slot is larger than max textures");
    return {};
  }

  m_textures.push_back(std::move(tex));

  if (!m_materialSet.writeTexture(slot, m_textures[slot])) {
    LOGE("Write texture failed");
    return {};
  }

  return TextureHandle{slot};
}

TextureHandle
MaterialSystem::loadTextureFromFile(VkUploadContext::Recorder staticRecorder,
                                    const std::string &path, bool flipY,
                                    TextureUsage usage) {
  engine::ImageData img;
  if (!engine::assets::loadImageRGBA8(path, img, flipY)) {
    LOGE("Failed to load image: {}", path);
    return {};
  }

  VkTexture2D tex;
  const VkFormat fmt = formatFor(usage);
  if (!m_textureUploader.uploadRGBA8(staticRecorder, img.pixels.data(), fmt,
                                     img.width, img.height, tex)) {
    LOGE("Texture from file creation failed");
    return {};
  }

  return storeTextureAndWrite(std::move(tex));
}

bool MaterialSystem::uploadTextureFromImage(
    VkUploadContext::Recorder staticRecorder, const engine::ImageData &img,
    TextureUsage usage, VkTexture2D &outTex) {
  if (!img.valid()) {
    LOGE("createTextureFromImage invalid image");
    return false;
  }

  const size_t expected = size_t(img.width) * img.height * 4ULL;
  if (img.pixels.size() != expected) {
    LOGE("Image byte size mismatch: have={} expected={}", img.pixels.size(),
         expected);
    return false;
  }

  const VkFormat fmt = formatFor(usage);
  return m_textureUploader.uploadRGBA8(staticRecorder, img.pixels.data(), fmt,
                                       img.width, img.height, outTex);
}

bool MaterialSystem::writeMaterialGPU(VkUploadContext::Recorder recorder,
                                      uint32_t materialId,
                                      const MaterialGPU &gpu) {
  if (m_materialTable == VK_NULL_HANDLE) {
    LOGE("Material table not bound");
    return false;
  }

  if (materialId == UINT32_MAX || materialId >= m_materialTableCapacity) {
    LOGE("Material id out of range");
    return false;
  }

  const VkDeviceSize dstOffset = VkDeviceSize(materialId) * sizeof(MaterialGPU);
  return MaterialUploader::uploadOne(recorder, m_materialTable, dstOffset, gpu);
}

uint32_t MaterialSystem::createMaterial(VkUploadContext::Recorder recorder,
                                        const MaterialDescription &desc) {
  if (!m_whiteTexture.valid() || !m_blackTexture.valid() ||
      !m_defaultMetalRough.valid() || !m_defaultOcclusion.valid() ||
      !m_defaultNormal.valid()) {
    LOGE("Defaults not initialized");
    return UINT32_MAX;
  }

  const uint32_t id = allocMaterialId();
  if (id == UINT32_MAX) {
    return UINT32_MAX;
  }

  // Resolve textures with defaults
  const TextureHandle baseColor =
      desc.baseColorTexture.value_or(m_whiteTexture);
  const TextureHandle emissive = desc.emissiveTexture.value_or(m_blackTexture);
  const TextureHandle metallicRoughness =
      desc.metallicRoughnessTexture.value_or(m_defaultMetalRough);
  const TextureHandle occlusion =
      desc.ambientOcclusionTexture.value_or(m_defaultOcclusion);
  const TextureHandle normal = desc.normal.value_or(m_defaultNormal);

  MaterialGPU gpu{};
  gpu.baseColorFactor = desc.baseColorFactor;
  gpu.emissiveFactor = {desc.emissiveFactor.x, desc.emissiveFactor.y,
                        desc.emissiveFactor.z, 0.0F};
  gpu.mrAoAlpha = {desc.metallicFactor, desc.roughnessFactor,
                   desc.ambientOcclusionFactor, desc.alphaCutoff};

  gpu.tex0.x = baseColor.id;
  gpu.tex1.x = emissive.id;

  gpu.tex0.y = normal.id;
  gpu.tex0.z = metallicRoughness.id;
  gpu.tex0.w = occlusion.id;

  if (!writeMaterialGPU(recorder, id, gpu)) {
    return UINT32_MAX;
  }

  return id;
}

bool MaterialSystem::updateMaterialGPU(VkUploadContext::Recorder recorder,
                                       uint32_t materialId,
                                       const MaterialGPU &gpu) {
  return writeMaterialGPU(recorder, materialId, gpu);
}

bool MaterialSystem::createDefaultMaterial(
    VkUploadContext::Recorder staticRecorder) noexcept {

  // BaseColor default: white (sRGB)
  {
    VkTexture2D tex;
    static constexpr std::array<std::uint8_t, 4> kWhiteRGBA8{255, 255, 255,
                                                             255};
    if (!m_textureUploader.uploadRGBA8(staticRecorder, kWhiteRGBA8.data(),
                                       VK_FORMAT_R8G8B8A8_SRGB, 1, 1, tex)) {
      LOGE("Default white texture creation failed");
      return false;
    }

    m_whiteTexture = storeTextureAndWrite(std::move(tex));
    if (m_whiteTexture.id == UINT32_MAX) {
      return false;
    }
  }

  // Emissive default: black (sRGB)
  {
    VkTexture2D tex;
    static constexpr std::array<uint8_t, 4> kBlack{0, 0, 0, 255};
    if (!m_textureUploader.uploadRGBA8(staticRecorder, kBlack.data(),
                                       VK_FORMAT_R8G8B8A8_SRGB, 1, 1, tex)) {
      LOGE("Default black texture creation failed");
      return false;
    }

    m_blackTexture = storeTextureAndWrite(std::move(tex));
    if (m_blackTexture.id == UINT32_MAX) {
      return false;
    }
  }

  // MetallicRoughness default (linear UNORM):
  // roughness = 1 in G, metallic = 0 in B. R unused, set 0. A = 255.
  {
    VkTexture2D tex;
    static constexpr std::array<uint8_t, 4> kMr{0, 255, 255, 255};
    if (!m_textureUploader.uploadRGBA8(staticRecorder, kMr.data(),
                                       VK_FORMAT_R8G8B8A8_UNORM, 1, 1, tex)) {
      LOGE("Default MetallicRoughness texture creation failed");
    }

    m_defaultMetalRough = storeTextureAndWrite(std::move(tex));
    if (m_defaultMetalRough.id == UINT32_MAX) {
      return false;
    }
  }

  // Occlusion default (linear UNORM): AO = 1 in R. (255, *, *, 255)
  {
    VkTexture2D tex;
    static constexpr std::array<uint8_t, 4> kAo{255, 255, 255, 255};
    if (!m_textureUploader.uploadRGBA8(staticRecorder, kAo.data(),
                                       VK_FORMAT_R8G8B8A8_UNORM, 1, 1, tex)) {
      LOGE("Default ambient occlusion creation failed");
      return false;
    }

    m_defaultOcclusion = storeTextureAndWrite(std::move(tex));
    if (m_defaultOcclusion.id == UINT32_MAX) {
      return false;
    }
  }

  // Normal default (linear UNORM): (0.5, 0.5, 1) => (128, 128, 255, 255)
  {
    VkTexture2D tex;
    static constexpr std::array<uint8_t, 4> kNormal{128, 128, 255, 255};
    if (!m_textureUploader.uploadRGBA8(staticRecorder, kNormal.data(),
                                       VK_FORMAT_R8G8B8A8_UNORM, 1, 1, tex)) {
      LOGE("Default normal creation failed");
      return false;
    }

    m_defaultNormal = storeTextureAndWrite(std::move(tex));
    if (m_defaultNormal.id == UINT32_MAX) {
      return false;
    }
  }

  {
    const uint32_t id = allocMaterialId();
    if (id == UINT32_MAX) {
      LOGE("Out of material table capacity or table not "
           "bound");
      return false;
    }

    MaterialSystem::MaterialDescription desc{};
    desc.baseColorFactor = {1, 1, 1, 1};
    desc.emissiveFactor = {0, 0, 0};
    desc.metallicFactor = 0.0F;
    desc.roughnessFactor = 1.0F;
    desc.ambientOcclusionFactor = 1.0F;
    desc.alphaCutoff = 0.5F;

    desc.baseColorTexture = m_whiteTexture;
    desc.emissiveTexture = m_blackTexture;
    desc.normal = m_defaultNormal;
    desc.metallicRoughnessTexture = m_defaultMetalRough;
    desc.ambientOcclusionTexture = m_defaultOcclusion;

    m_defaultMaterial = createMaterial(staticRecorder, desc);
    return m_defaultMaterial != UINT32_MAX;
  }

  return true;
}

void MaterialSystem::bindTextureTable(VkCommandBuffer cmd,
                                      VkPipelineLayout layout,
                                      uint32_t setIndex) const {
  m_materialSet.bind(cmd, layout, setIndex);
}

void MaterialSystem::bindMaterialTable(VkBuffer materialTableBuffer,
                                       uint32_t maxMaterialsInTable) noexcept {
  m_materialTable = materialTableBuffer;
  m_materialTableCapacity = maxMaterialsInTable;
}
