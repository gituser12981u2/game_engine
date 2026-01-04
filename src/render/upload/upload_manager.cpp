#include "render/upload/upload_manager.hpp"

#include <cstdint>
#include <iostream>

bool UploadManager::init(VkBackendCtx &ctx, uint32_t framesInFlight,
                         VkDeviceSize staticBudgetPerFrame,
                         VkDeviceSize frameBudgetPerFrame,
                         UploadProfiler *profiler) {
  if (framesInFlight == 0 || staticBudgetPerFrame == 0 ||
      frameBudgetPerFrame == 0) {
    std::cerr << "[UploadManager] init invalid args\n";
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_framesInFlight = framesInFlight;

  if (!m_static.init(ctx, m_framesInFlight, staticBudgetPerFrame, profiler)) {
    std::cerr << "[Renderer] Failed to init static upload context\n";
    shutdown();
    return false;
  }

  if (!m_frame.init(ctx, m_framesInFlight, frameBudgetPerFrame, profiler)) {
    std::cerr << "[Renderer] Failed to init frame upload context\n";
    shutdown();
    return false;
  }

  return true;
}

void UploadManager::shutdown() noexcept {
  m_frame.shutdown();
  m_static.shutdown();
  m_ctx = nullptr;
  m_framesInFlight = 0;
}

bool UploadManager::beginFrame(uint32_t frameIndex) {
  if (m_ctx == nullptr) {
    return false;
  }

  // Begin both lanes for the same frame index
  if (!m_static.beginFrame(frameIndex)) {
    return false;
  }

  return m_frame.beginFrame(frameIndex);
}

bool UploadManager::flushFrame(bool wait) { return m_frame.flush(wait); }
bool UploadManager::flushStatic(bool wait) { return m_static.flush(wait); }

bool UploadManager::flushAll(bool wait) {
  if (!flushFrame(wait)) {
    return false;
  }

  return flushStatic(wait);
}
