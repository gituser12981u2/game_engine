#include "camera_controller.hpp"

#include "engine/camera/camera.hpp"
#include "platform/window/glfw_window.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>

CameraController::CameraController(GlfwWindow &window, Camera *camera)
    : m_window(&window), m_camera(camera) {}

void CameraController::enableCursorCapture(bool enabled) {
  GLFWwindow *w = glfw();
  if (w == nullptr) {
    return;
  }

  glfwSetInputMode(w, GLFW_CURSOR,
                   enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

  m_cursorCaptured = enabled;
  m_firstMouse = true;
}

void CameraController::update(float dtSeconds) {
  GLFWwindow *w = glfw();
  if (w == nullptr || m_camera == nullptr) {
    return;
  }

  // Re-lock cursor on left mouse click
  const bool mouseLeft =
      glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  if (!m_cursorCaptured && mouseLeft && !m_prevMouseLeft) {
    enableCursorCapture(true);
  }

  m_prevMouseLeft = mouseLeft;

  handleMouseLook();
  handleKeyboard(dtSeconds);
}

void CameraController::handleMouseLook() {
  GLFWwindow *w = glfw();

  if (w == nullptr) {
    return;
  }

  if (glfwGetInputMode(w, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
    return;
  }

  double x;
  double y;
  glfwGetCursorPos(w, &x, &y);

  if (m_firstMouse) {
    m_lastX = x;
    m_lastY = y;
    m_firstMouse = false;
    return;
  }

  const double dx = x - m_lastX;
  const double dy = y - m_lastY;
  m_lastX = x;
  m_lastY = y;

  // yaw: -dx, pitch: -dy
  m_camera->addYawPitch(static_cast<float>(-dx) * mouseSesnitivity,
                        static_cast<float>(-dy) * mouseSesnitivity);
}

void CameraController::handleKeyboard(float dtSeconds) {
  GLFWwindow *w = glfw();

  if (w == nullptr || m_camera == nullptr) {
    return;
  }

  float speed = moveSpeed;
  if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    speed *= sprintMult;
  }

  const float velocity = speed * dtSeconds;

  const glm::vec3 f = m_camera->forward();
  const glm::vec3 r = m_camera->right();
  const glm::vec3 u(0.0F, 0.0F, 1.0F);

  // WASD movement
  if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) {
    m_camera->position += f * velocity;
  }

  if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) {
    m_camera->position -= f * velocity;
  }

  if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) {
    m_camera->position += r * velocity;
  }

  if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) {
    m_camera->position -= r * velocity;
  }

  // Vertical movement
  if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) {
    m_camera->position += u * velocity;
  }

  if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) {
    m_camera->position -= u * velocity;
  }

  // Toggle cursor escape
  if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    enableCursorCapture(false);
  }
}
