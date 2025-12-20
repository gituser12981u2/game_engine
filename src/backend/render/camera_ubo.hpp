#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include <glm/glm.hpp>

struct CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
};

static_assert(sizeof(CameraUBO) % 16 == 0);
