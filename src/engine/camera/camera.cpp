#include "camera.hpp"
#include "camera_ubo.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include <algorithm>
#include <cmath>
#include <vulkan/vulkan_core.h>

static float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(v, hi));
}

glm::vec3 Camera::forward() const {
  const float yaw = glm::radians(yawDeg);
  const float pitch = glm::radians(pitchDeg);

  glm::vec3 forward;
  forward.x = std::cos(pitch) * std::cos(yaw);
  forward.y = std::cos(pitch) * std::sin(yaw);
  forward.z = std::sin(pitch);

  return glm::normalize(forward);
}

glm::vec3 Camera::right() const {
  // Z-up world
  const glm::vec3 worldUp(0.0F, 0.0F, 1.0F);
  return glm::normalize(glm::cross(forward(), worldUp));
}

glm::vec3 Camera::up() const {
  return glm::normalize(glm::cross(right(), forward()));
}

void Camera::addYawPitch(float dYawDeg, float dPitchDeg) {
  yawDeg += dYawDeg;
  pitchDeg = clampf(pitchDeg + dPitchDeg, -89.0F, 89.0F);
}

CameraUBO Camera::makeUbo(VkExtent2D extent) const {
  CameraUBO ubo{};

  const glm::vec3 f = forward();
  ubo.view = glm::lookAt(position, position + f, glm::vec3(0.0F, 0.0F, 1.0F));

  const float width = static_cast<float>(extent.width);
  const float height =
      static_cast<float>(extent.height > 0 ? extent.height : 1U);
  const float aspect = width / height;

  ubo.proj = glm::perspective(glm::radians(fovDeg), aspect, zNear, zFar);
  ubo.proj[1][1] *= -1.0F; // Vulkan clip space

  return ubo;
}
