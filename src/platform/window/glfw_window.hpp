#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <vector>

class GlfwWindow {
public:
  GlfwWindow() = default;
  ~GlfwWindow() noexcept { shutdown(); }

  GlfwWindow(const GlfwWindow &) = delete;
  GlfwWindow &operator=(const GlfwWindow &) = delete;

  GlfwWindow(GlfwWindow &&o) noexcept { *this = std::move(o); }
  GlfwWindow &operator=(GlfwWindow &&o) noexcept {
    if (this == &o) {
      return *this;
    }

    shutdown();

    m_window = o.m_window;
    o.m_window = nullptr;
    m_inited = o.m_inited;
    o.m_inited = false;

    return *this;
  }

  bool init(uint32_t width, uint32_t height, const char *title);
  void shutdown() noexcept;

  [[nodiscard]] GLFWwindow *handle() const noexcept { return m_window; }
  [[nodiscard]] bool valid() const noexcept { return m_window != nullptr; }

  [[nodiscard]] bool shouldClose() const noexcept;

  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  void pollEvents() const noexcept;

  void framebufferSize(uint32_t &outWidth, uint32_t &outHeight) const;

  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  [[nodiscard]] std::vector<const char *> requiredVulkanExtensions();

private:
  GLFWwindow *m_window = nullptr;
  bool m_inited = false;
};
