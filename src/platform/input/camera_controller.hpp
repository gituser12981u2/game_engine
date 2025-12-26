#pragma once

#include "engine/camera/camera.hpp"
#include "platform/window/glfw_window.hpp"

class CameraController {
public:
  explicit CameraController(GlfwWindow &window, Camera *camera);

  void enableCursorCapture(bool enabled);

  void update(float dtSeconds);

  float moveSpeed = 3.5F;         // units per sec
  float sprintMult = 3.0F;        // On SHIFT hold
  float mouseSesnitivity = 0.10F; // deg per pixel

private:
  GlfwWindow *m_window = nullptr; // non-owning
  Camera *m_camera = nullptr;     // non-owning

  bool m_firstMouse = true;
  double m_lastX = 0.0;
  double m_lastY = 0.0;

  bool m_cursorCaptured = false;
  bool m_prevMouseLeft = false;

  [[nodiscard]] GLFWwindow *glfw() const noexcept {
    return (m_window != nullptr) ? m_window->handle() : nullptr;
  }

  void handleMouseLook();
  void handleKeyboard(float dtSeconds);
};
