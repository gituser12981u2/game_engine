#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

class GlfwWindow {
public:
  GlfwWindow() = default;
  ~GlfwWindow() noexcept { shutdown(); }

  GlfwWindow(const GlfwWindow &) = delete;
  GlfwWindow &operator=(const GlfwWindow &) = delete;

  GlfwWindow(GlfwWindow &&other) noexcept { *this = std::move(other); }
  GlfwWindow &operator=(GlfwWindow &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    shutdown();

    m_window = other.m_window;
    other.m_window = nullptr;
    m_inited = other.m_inited;
    other.m_inited = false;

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

  bool createVulkanSurface(VkInstance instance, VkSurfaceKHR &outSurface) const;

private:
  GLFWwindow *m_window = nullptr;
  bool m_inited = false;
};
