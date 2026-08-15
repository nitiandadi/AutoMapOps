#include "automap/core/lane_boundary.hpp"

namespace automap::core {

std::string_view lane_boundary_type_name(LaneBoundaryType type) noexcept {
    switch (type) {
        case LaneBoundaryType::unknown:
            return "unknown";
        case LaneBoundaryType::dashed_line:
            return "dashed_line";
        case LaneBoundaryType::solid_line:
            return "solid_line";
        case LaneBoundaryType::double_solid_line:
            return "double_solid_line";
        case LaneBoundaryType::curb:
            return "curb";
        case LaneBoundaryType::virtual_boundary:
            return "virtual_boundary";
    }

    return "unknown";
}

}  // namespace automap::core
