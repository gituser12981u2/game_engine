#pragma once

#include "backend/gpu/upload/vk_upload_context.hpp"
#include "render/resources/material_gpu.hpp"

#include <vulkan/vulkan_core.h>

namespace MaterialUploader {

bool uploadOne(
    VkUploadContext::Recorder recorder, VkBuffer materialBuffer,
    VkDeviceSize dstOffsetBytes, const MaterialGPU &material,
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

}
