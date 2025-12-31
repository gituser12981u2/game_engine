#pragma once

#include <type_traits>
#include <utility>

template <class F> class ScopeExit {
public:
  static_assert(std::is_nothrow_move_constructible_v<F>,
                "ScopeExit requires nothrow-movable functor");

  explicit ScopeExit(F &&f) noexcept(std::is_nothrow_move_constructible_v<F>)
      : m_fn(std::forward<F>(f)), m_active(true) {}

  ScopeExit(const ScopeExit &) = delete;
  ScopeExit &operator=(const ScopeExit &) = delete;

  ScopeExit(ScopeExit &&other) noexcept(std::is_nothrow_move_constructible_v<F>)
      : m_fn(std::move(other.m_fn)), m_active(other.m_active) {
    other.m_active = false;
  }

  ScopeExit &operator=(ScopeExit &&other) noexcept(
      std::is_nothrow_move_constructible_v<F> &&
      std::is_nothrow_move_assignable_v<F>) {
    if (this == &other) {
      return *this;
    }
    if (m_active) {
      m_fn();
    }
    m_fn = std::move(other.m_fn);
    m_active = other.m_active;
    other.m_active = false;
    return *this;
  }

  ~ScopeExit() noexcept {
    if (m_active) {
      m_fn();
    }
  }

  void dismiss() noexcept { m_active = false; }

private:
  F m_fn;
  bool m_active = false;
};

template <class F>
[[nodiscard]] ScopeExit<std::decay_t<F>> makeScopeExit(F &&f) noexcept(
    std::is_nothrow_move_constructible_v<std::decay_t<F>>) {
  return ScopeExit<std::decay_t<F>>(std::forward<F>(f));
}
