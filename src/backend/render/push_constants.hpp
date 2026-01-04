#pragma once

#include <cstdint>

struct DrawPushConstants {
  uint32_t baseInstance = 0;
  uint32_t materialId = 0;
};

static_assert(sizeof(DrawPushConstants) == 8);
