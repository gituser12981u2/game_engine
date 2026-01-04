#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint4.hpp>

struct MaterialGPU {
  static constexpr uint32_t kNoTexture = 0xFFFFFFFFU;

  glm::vec4 baseColorFactor{1.0F}; // rgba
  glm::vec4 emissiveFactor{0.0F};  // rgb, w unused

  // x=metallic, y=roughness, z=aoStrength, w=alphaCutoff
  glm::vec4 mrAoAlpha{0.0F, 1.0F, 1.0F, 0.5F};

  // texture indices
  // x=baseColor, y=normal, z=metalRough, w=occlusion
  glm::uvec4 tex0{kNoTexture, kNoTexture, kNoTexture, kNoTexture};

  // x=emissive, y/z/w reserved
  glm::uvec4 tex1{kNoTexture, kNoTexture, kNoTexture, kNoTexture};

  // flags (alpha mode, double-sided, etc.)
  glm::uvec4 flags{0U, 0U, 0U, 0U};
};

// multiple of 16 bytes for for std430
static_assert(sizeof(MaterialGPU) % 16 == 0);
