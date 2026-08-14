#include "automap/core/geometry.hpp"

#include <algorithm>
#include <cmath>

namespace automap::core {

bool BoundingBox3d::contains(const Point3d& point, double epsilon) const noexcept {
    return point.x >= min.x - epsilon && point.x <= max.x + epsilon &&
           point.y >= min.y - epsilon && point.y <= max.y + epsilon &&
           point.z >= min.z - epsilon && point.z <= max.z + epsilon;
}

bool almost_equal(double lhs, double rhs, double epsilon) noexcept {
    if (lhs == rhs) {
        return true;
    }

    const double scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});
    return std::abs(lhs - rhs) <= epsilon * scale;
}

double distance(const Point3d& lhs, const Point3d& rhs) noexcept {
    return std::hypot(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

bool points_coincident(
    const Point3d& lhs,
    const Point3d& rhs,
    double epsilon_m) noexcept {
    return distance(lhs, rhs) <= epsilon_m;
}

double polyline_length(const Polyline3d& polyline) noexcept {
    double length = 0.0;
    for (std::size_t index = 1; index < polyline.size(); ++index) {
        length += distance(polyline[index - 1], polyline[index]);
    }
    return length;
}

std::optional<BoundingBox3d> bounding_box(const Polyline3d& polyline) noexcept {
    if (polyline.empty()) {
        return std::nullopt;
    }

    BoundingBox3d bounds{.min = polyline.front(), .max = polyline.front()};
    for (const Point3d& point : polyline) {
        bounds.min.x = std::min(bounds.min.x, point.x);
        bounds.min.y = std::min(bounds.min.y, point.y);
        bounds.min.z = std::min(bounds.min.z, point.z);
        bounds.max.x = std::max(bounds.max.x, point.x);
        bounds.max.y = std::max(bounds.max.y, point.y);
        bounds.max.z = std::max(bounds.max.z, point.z);
    }
    return bounds;
}

}  // namespace automap::core

