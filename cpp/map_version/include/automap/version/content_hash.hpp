#pragma once

#include "automap/core/map_data.hpp"

#include <string>
#include <string_view>

namespace automap::version {

[[nodiscard]] std::string sha256_hex(std::string_view content);

// 对紧凑、确定性写出的 Canonical JSON 计算 SHA-256。
[[nodiscard]] std::string canonical_content_hash(const core::MapData& map);

}  // namespace automap::version
