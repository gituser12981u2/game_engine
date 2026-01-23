#pragma once

#include "engine/camera/camera_ubo.hpp"
#include "render/scene/debug_ubo.hpp"

struct SceneUBO {
  CameraUBO camera;
  DebugUBO debug;
};
