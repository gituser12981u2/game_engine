#include "stb_image_loader.hpp"

#include "engine/assets/image_data.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stb_image.h>

namespace engine::assets {

bool loadImageRGBA8(const std::string &path, ImageData &out, bool flipY) {
  out = {};

  stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

  int width = 0;
  int height = 0;
  int numChannels = 0;
  stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &numChannels,
                              STBI_rgb_alpha); // force RGBA
  if (pixels == nullptr || width <= 0 || height <= 0) {
    std::cerr << "[Image] stbi_load failed: "
              << ((stbi_failure_reason() != nullptr) ? stbi_failure_reason()
                                                     : "unknown")
              << "\n";
    return false;
  }

  out.width = static_cast<uint32_t>(width);
  out.height = static_cast<uint32_t>(height);
  out.channels = static_cast<uint32_t>(numChannels);

  const size_t byteCount =
      static_cast<size_t>(out.width) * static_cast<size_t>(out.height) * 4ULL;
  out.pixels.assign(pixels, pixels + byteCount);

  stbi_image_free(pixels);
  return out.valid();
}

} // namespace engine::assets
