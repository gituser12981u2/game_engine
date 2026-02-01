#pragma once

#include <functional>

namespace editor {

class Editor {
public:
  using BuildFn = std::function<void()>;

  Editor() = default;
  ~Editor() = default;

  Editor(const Editor &) = delete;
  Editor &operator=(const Editor &) = delete;
  Editor(Editor &&) = delete;
  Editor &operator=(Editor &&) = delete;

  void tick();

  [[nodiscard]] const BuildFn &buildFn() const noexcept { return m_build; }

private:
  void buildPanels();

  BuildFn m_build = [this] { buildPanels(); };
};

} // namespace editor
