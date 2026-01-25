#pragma once

#include "engine/camera/camera_ubo.hpp"
#include "render/scene/debug_ubo.hpp"
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <sys/stat.h>

static constexpr uint32_t kMaxDirLights = 4;

struct alignas(16) DirectionalLight {
  glm::vec4 directionWS_illuminanceLux; // xyz dir, w lux

  glm::vec4 colorLinear_pad; // rgb color, w pad
};

static_assert(sizeof(DirectionalLight) == 32);

static_assert(sizeof(DirectionalLight) == 32);

struct alignas(16) SceneUBO {
  CameraUBO camera;
  DebugUBO debug;

  uint32_t pointLightCount;
  uint32_t dirLightCount;

  uint32_t padA;
  uint32_t padB;

  std::array<DirectionalLight, kMaxDirLights> dirLights;
};

static_assert(sizeof(SceneUBO) % 16 == 0);
