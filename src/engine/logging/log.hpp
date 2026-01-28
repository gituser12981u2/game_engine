#pragma once

#include <memory>
#include <spdlog/logger.h>
#include <string_view>

namespace logging {

void init();
void shutdown();

std::shared_ptr<spdlog::logger> &engine();
std::shared_ptr<spdlog::logger> get(std::string_view name);
} // namespace logging

#define LOG_TU_LOGGER() (::logging::engine())

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
