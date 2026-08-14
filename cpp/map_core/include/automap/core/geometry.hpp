#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace automap::core {

namespace tolerance {

inline constexpr double floating_point = 1e-6;
inline constexpr double coincident_point_m = 1e-3;

}  // namespace tolerance

struct Point3d final {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    bool operator==(const Point3d&) const = default;
};

using Polyline3d = std::vector<Point3d>;

struct BoundingBox3d final {
    Point3d min;
    Point3d max;

    [[nodiscard]] bool contains(
        const Point3d& point,
        double epsilon = tolerance::floating_point) const noexcept;
};

[[nodiscard]] bool almost_equal(
    double lhs,
    double rhs,
    double epsilon = tolerance::floating_point) noexcept;

[[nodiscard]] bool points_coincident(
    const Point3d& lhs,
    const Point3d& rhs,
    double epsilon_m = tolerance::coincident_point_m) noexcept;

[[nodiscard]] double distance(const Point3d& lhs, const Point3d& rhs) noexcept;

[[nodiscard]] double polyline_length(const Polyline3d& polyline) noexcept;

[[nodiscard]] std::optional<BoundingBox3d> bounding_box(
    const Polyline3d& polyline) noexcept;

}  // namespace automap::core

