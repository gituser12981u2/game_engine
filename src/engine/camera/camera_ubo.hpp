#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

struct alignas(16) CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
  glm::vec4 cameraPosWS_pad; // xyz = cameraPosWS, w = pad
};

static_assert(sizeof(CameraUBO) % 16 == 0);
