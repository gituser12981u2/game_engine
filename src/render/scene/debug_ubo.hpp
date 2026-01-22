#pragma once

#include <cstdint>

struct alignas(16) DebugUBO {
  uint32_t view = 0;  // 0=final, 1=baseColor, 2=metallic, 3=roughness, 4=ao,
                      // 5=emissive, 6=vertex color, 7=uv
  uint32_t flags = 0; // bitfield toggles
  float value0 = 0.0F;
  float value1 = 0.0F;
};

static_assert(sizeof(DebugUBO) == 16);
static_assert(alignof(DebugUBO) == 16);
