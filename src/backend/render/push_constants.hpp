#pragma once

#include <cstdint>

struct DrawPushConstants {
  std::uint32_t baseInstance = 0;
};

static_assert(sizeof(DrawPushConstants) == 4);
