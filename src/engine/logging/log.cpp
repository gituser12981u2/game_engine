#include "engine/logging/log.hpp"

#include <glm/detail/type_quat.hpp>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace log {

// Pattern: time | level | thread | logger | msg
static constexpr const char *kPattern =
    "%Y-%m-%d %H:%M:%S.%e | %^%l%$ | t:%t | %n | %s:%# | %v";

static std::shared_ptr<spdlog::logger> quark;

static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> g_consoleSink;
static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> g_fileSink;

void init() {
  if (quark) {
    return;
  }

  g_consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  g_fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "quark.log",
      10 * 1024 * 1024, // 10 MB
      3                 // keep 3 old logs
  );

  quark = std::make_shared<spdlog::logger>(
      "Quark", spdlog::sinks_init_list{g_consoleSink, g_fileSink});

  quark->set_pattern(kPattern);

#ifndef NDEBUG
  quark->set_level(spdlog::level::trace);
#else
  quark->set_level(spdlog::level::info);
#endif

  quark->flush_on(spdlog::level::warn);

  spdlog::register_logger(quark);
  spdlog::set_default_logger(quark);
}

void shutdown() {
  if (quark) {
    quark->flush();
    quark.reset();
  }

  g_consoleSink.reset();
  g_fileSink.reset();
  spdlog::drop_all();
}

std::shared_ptr<spdlog::logger> &engine() { return quark; }

std::shared_ptr<spdlog::logger> get(std::string_view name) {
  if (!quark) {
    init();
  }

  if (auto existing = spdlog::get(std::string{name}); existing) {
    return existing;
  }

  auto logger = std::make_shared<spdlog::logger>(
      std::string{name}, spdlog::sinks_init_list{g_consoleSink, g_fileSink});

  logger->set_pattern(kPattern);
  logger->set_level(quark->level());
  logger->flush_on(spdlog::level::warn);

  spdlog::register_logger(logger);
  return logger;
}

} // namespace log
