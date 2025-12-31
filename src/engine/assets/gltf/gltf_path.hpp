#pragma once

#include <string>

namespace engine::assets {

inline std::string dirOf(const std::string &path) {
  const auto pos = path.find_last_of("/\\");
  return (pos == std::string::npos) ? std::string{} : path.substr(0, pos + 1);
}

static bool isAbsoluteOrDataUri(const std::string &uri) {
  if (uri.starts_with("data:")) {
    return true;
  }

  if (uri.find("://") != std::string::npos) {
    return true;
  }

#if defined(_WIN32)
  if (uri.size() >= 2 && std::isalpha((unsigned char)uri[0]) && uri[1] == ':') {
    return true;
  }
#endif

  return !uri.empty() && (uri[0] == '/' || uri[0] == '\\');
}

inline std::string resolveUriRelativeToFile(const std::string &gltfPath,
                                            const std::string &uri) {
  if (uri.empty()) {
    return {};
  }

  if (isAbsoluteOrDataUri(uri)) {
    return uri;
  }

  return dirOf(gltfPath) + uri;
}

} // namespace engine::assets
