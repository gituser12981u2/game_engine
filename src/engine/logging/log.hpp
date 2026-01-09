#pragma once

#include <memory>
#include <spdlog/logger.h>
#include <string_view>

namespace log {

void init();
void shutdown();

std::shared_ptr<spdlog::logger> &engine();

std::shared_ptr<spdlog::logger> get(std::string_view name);

inline std::shared_ptr<spdlog::logger> render() { return get("Render"); }
inline std::shared_ptr<spdlog::logger> backend() { return get("Backend"); }

} // namespace log

// #ifndef LOG_TU_LOGGER
// #define LOG_TU_LOGGER() ::log::engine()
// #endif

#define DEFINE_TU_LOGGER(name)                                                 \
  static inline std::shared_ptr<spdlog::logger> ThisLogger() {                 \
    static auto lg = ::log::get(name);                                         \
    return lg;                                                                 \
  }                                                                            \
  static_assert(true)

// Default (Engine) logger
#define LOG_TRACE(...) ::log::engine()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::log::engine()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::log::engine()->info(__VA_ARGS__)
#define LOG_WARN(...)                                                          \
  ::log::engine()->warn(                                                       \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  ::log::engine()->error(                                                      \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_CRIT(...)                                                          \
  ::log::engine()->critical(                                                   \
      spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

// Logger-explicit
#define LOGT(...) (LOG_TU_LOGGER())->trace(__VA_ARGS__)
#define LOGD(...) (LOG_TU_LOGGER())->debug(__VA_ARGS__)
#define LOGI(...) (LOG_TU_LOGGER())->info(__VA_ARGS__)
#define LOGW(...)                                                              \
  (LOG_TU_LOGGER())                                                            \
      ->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},           \
            spdlog::level::warn, __VA_ARGS__)

#define LOGE(...)                                                              \
  (LOG_TU_LOGGER())                                                            \
      ->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},           \
            spdlog::level::err, __VA_ARGS__)

#define LOGC(...)                                                              \
  (LOG_TU_LOGGER())                                                            \
      ->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},           \
            spdlog::level::critical, __VA_ARGS__)
