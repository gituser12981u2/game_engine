#include "render/upload/upload_manager.hpp"

#include "backend/gpu/upload/vk_upload_context.hpp"

#include <cstdint>
#include <iostream>

bool UploadManager::init(VkBackendCtx &ctx, uint32_t framesInFlight,
                         VkDeviceSize staticTotalBytes,
                         VkDeviceSize frameBudget, uint32_t threadCount,
                         UploadProfiler *profiler) {
  if (framesInFlight == 0 || staticTotalBytes == 0 || frameBudget == 0 ||
      threadCount == 0) {
    std::cerr << "[UploadManager] init invalid args\n";
    return false;
  }

  shutdown();

  m_ctx = &ctx;
  m_framesInFlight = framesInFlight;
  m_threadCount = threadCount;

  m_currentFrameIndex = 0;
  m_frameBegun = false;

  m_staticActive = false;

  if (!m_static.initOneShot(ctx, staticTotalBytes, threadCount, profiler)) {
    std::cerr << "[Renderer] Failed to init static upload context\n";
    shutdown();
    return false;
  }

  if (!m_frame.initFrameRing(ctx, m_framesInFlight, frameBudget, threadCount,
                             profiler)) {
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
  m_threadCount = 0;

  m_currentFrameIndex = 0;
  m_frameBegun = false;

  m_staticActive = false;
}

bool UploadManager::beginFrame(uint32_t frameIndex) {
  if (m_ctx == nullptr) {
    return false;
  }

  if (frameIndex >= m_framesInFlight) {
    std::cerr << "[UploadManager] beginFrame frameIndex out of range\n";
    return false;
  }

  m_currentFrameIndex = frameIndex;
  m_frameBegun = false;

  if (!m_frame.beginFrame(frameIndex)) {
    std::cerr << "[UploadManager] frame.beginFrame failed\n";
    return false;
  }

  m_frameBegun = true;
  return true;
}

bool UploadManager::flushFrame(bool wait) {
  if (m_ctx == nullptr) {
    return false;
  }

  if (!m_frameBegun) {
    return true;
  }

  const bool ok = m_frame.flushFrame(m_currentFrameIndex, wait);
  m_frameBegun = false;
  return ok;
}

VkUploadContext::Recorder UploadManager::frameRecorder(uint32_t threadIndex) {
  if (m_ctx == nullptr || !m_frameBegun) {
    return {};
  }

  return m_frame.recorder(m_currentFrameIndex, threadIndex);
}

bool UploadManager::beginStatic() {
  if (m_ctx == nullptr) {
    return false;
  }

  if (m_staticActive) {
    std::cerr
        << "[UploadManager] beginStatic called while static batch active\n";
    return false;
  }

  if (!m_static.beginBatch()) {
    std::cerr << "[UploadManager] static.beginBatch failed\n";
    return false;
  }

  m_staticActive = true;
  return true;
}

bool UploadManager::flushStatic(bool wait) {
  if (m_ctx == nullptr) {
    return false;
  }

  if (!m_staticActive) {
    return true;
  }

  const bool ok = m_static.flushBatch(wait);
  m_staticActive = false;
  return ok;
}

VkUploadContext::Recorder UploadManager::staticRecorder(uint32_t threadIndex) {
  if (m_ctx == nullptr || !m_staticActive) {
    return {};
  }

  return m_static.recorder(/*frameIndex=*/0, threadIndex);
}

bool UploadManager::flushAll(bool wait) {
  if (!flushFrame(wait)) {
    return false;
  }

  return flushStatic(wait);
}
