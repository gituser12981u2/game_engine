#pragma once

#include "../../engine/camera/camera.hpp"
#include <GLFW/glfw3.h>

class CameraController {
public:
  explicit CameraController(GLFWwindow *window, Camera *camera);

  void enableCursorCapture(bool enabled);

  void update(float dtSeconds);

  float moveSpeed = 3.5F;         // units per sec
  float sprintMult = 3.0F;        // On SHIFT hold
  float mouseSesnitivity = 0.10F; // deg per pixel

private:
  GLFWwindow *m_window = nullptr; // non-owning
  Camera *m_camera = nullptr;     // non-owning

  bool m_firstMouse = true;
  double m_lastX = 0.0;
  double m_lastY = 0.0;

  void handleMouseLook();
  void handleKeyboard(float dtSeconds);
};
