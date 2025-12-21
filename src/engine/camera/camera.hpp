#pragma once

#include "camera_ubo.hpp"
#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan_core.h>

class Camera {
public:
  // World-space state
  glm::vec3 position{2.0F, 2.0F, 2.0F};
  float yawDeg = -135.0F;
  float pitchDeg = -35.0F;

  // Projection params
  float fovDeg = 60.0F;
  float zNear = 0.1F;
  float zFar = 100.0F;

  [[nodiscard]] glm::vec3 forward() const;
  [[nodiscard]] glm::vec3 right() const;
  [[nodiscard]] glm::vec3 up() const;

  [[nodiscard]] CameraUBO makeUbo(VkExtent2D extent) const;

  void addYawPitch(float dYawDeg, float dPitchDeg);
};
