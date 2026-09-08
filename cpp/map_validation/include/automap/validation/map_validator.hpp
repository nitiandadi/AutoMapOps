#pragma once

#include "automap/core/map_data.hpp"
#include "automap/validation/validation_report.hpp"

namespace automap::validation {

// 按固定顺序执行当前版本的全部地图质检规则。
[[nodiscard]] ValidationReport validate_map(const core::MapData& map);

}  // namespace automap::validation
