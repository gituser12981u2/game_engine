#pragma once

#include <glm/ext/vector_float3.hpp>

struct alignas(16) PointLightGPU {
  glm::vec3 positionWS{0.0F};
  float radius{1.0F};

  glm::vec3 colorLinear{1.0F};
  float lumens{0.0F};
};

static_assert(sizeof(PointLightGPU) == 32);
static_assert(alignof(PointLightGPU) >= 16);
