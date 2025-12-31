#pragma once

#include "engine/assets/image_data.hpp"

#include <string>

namespace engine::assets {

bool loadImageRGBA8(const std::string &path, ImageData &out, bool flipY = true);

}
