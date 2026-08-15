#pragma once

#include "automap/core/geometry.hpp"
#include "automap/core/object_id.hpp"

#include <string_view>

namespace automap::core {

// Physical or logical form of a lane boundary. Crossing permission is kept as
// a separate attribute because traffic rules cannot always be inferred from
// the painted or physical form alone.
enum class LaneBoundaryType {
    unknown,
    dashed_line,
    solid_line,
    double_solid_line,
    curb,
    virtual_boundary,
};

[[nodiscard]] std::string_view lane_boundary_type_name(
    LaneBoundaryType type) noexcept;

struct LaneBoundary final {
    ObjectId id;
    Polyline3d geometry;
    LaneBoundaryType type{LaneBoundaryType::unknown};
    bool crossing_allowed{false};

    bool operator==(const LaneBoundary&) const = default;
};

}  // namespace automap::core
