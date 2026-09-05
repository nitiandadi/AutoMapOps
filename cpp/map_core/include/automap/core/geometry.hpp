#pragma once

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <variant>
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

enum class PathTraversal {
    forward,
    reverse,
};

struct LineCurveSegment3d final {
    Point3d start;
    double heading_rad{0.0};
    double length_m{0.0};
    double end_z_m{0.0};

    bool operator==(const LineCurveSegment3d&) const = default;
};

struct CircularArcSegment3d final {
    Point3d start;
    double heading_rad{0.0};
    double length_m{0.0};
    double end_z_m{0.0};
    double curvature_per_m{0.0};

    bool operator==(const CircularArcSegment3d&) const = default;
};

struct ClothoidSegment3d final {
    Point3d start;
    double heading_rad{0.0};
    double length_m{0.0};
    double end_z_m{0.0};
    double start_curvature_per_m{0.0};
    double end_curvature_per_m{0.0};

    bool operator==(const ClothoidSegment3d&) const = default;
};

using CurveSegment3d = std::variant<
    LineCurveSegment3d,
    CircularArcSegment3d,
    ClothoidSegment3d>;

struct CompositeCurve3d final {
    std::vector<CurveSegment3d> segments;

    bool operator==(const CompositeCurve3d&) const = default;
};

class PathGeometry3d final {
public:
    using Storage = std::variant<Polyline3d, CompositeCurve3d>;

    PathGeometry3d() = default;
    PathGeometry3d(Polyline3d polyline);
    PathGeometry3d(std::initializer_list<Point3d> points);
    PathGeometry3d(CompositeCurve3d curve);

    [[nodiscard]] bool is_polyline() const noexcept;
    [[nodiscard]] bool is_composite_curve() const noexcept;
    [[nodiscard]] const Polyline3d* polyline() const noexcept;
    [[nodiscard]] Polyline3d* polyline() noexcept;
    [[nodiscard]] const CompositeCurve3d* composite_curve() const noexcept;
    [[nodiscard]] CompositeCurve3d* composite_curve() noexcept;
    [[nodiscard]] const Storage& storage() const noexcept;

    bool operator==(const PathGeometry3d&) const = default;

private:
    Storage storage_{Polyline3d{}};
};

struct CurveState3d final {
    Point3d position;
    double heading_rad{0.0};
    double curvature_per_m{0.0};

    bool operator==(const CurveState3d&) const = default;
};

struct TessellationOptions final {
    double max_chord_error_m{0.01};
    double max_segment_length_m{5.0};
    std::size_t max_recursion_depth{20U};
};

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

[[nodiscard]] double polyline_length(const PathGeometry3d& path) noexcept;

[[nodiscard]] std::optional<BoundingBox3d> bounding_box(
    const Polyline3d& polyline) noexcept;

[[nodiscard]] std::optional<BoundingBox3d> bounding_box(
    const PathGeometry3d& path);

[[nodiscard]] std::optional<BoundingBox3d> bounding_box(
    std::initializer_list<Point3d> points) noexcept;

[[nodiscard]] double normalize_heading(double heading_rad) noexcept;

[[nodiscard]] double curve_segment_length(const CurveSegment3d& segment) noexcept;

[[nodiscard]] CurveState3d evaluate_curve_segment(
    const CurveSegment3d& segment,
    double s) noexcept;

[[nodiscard]] double path_planar_length(const PathGeometry3d& path) noexcept;

[[nodiscard]] CurveState3d evaluate_path_geometry(
    const PathGeometry3d& path,
    double s,
    PathTraversal traversal = PathTraversal::forward) noexcept;

[[nodiscard]] CurveState3d path_start_state(
    const PathGeometry3d& path,
    PathTraversal traversal = PathTraversal::forward) noexcept;

[[nodiscard]] CurveState3d path_end_state(
    const PathGeometry3d& path,
    PathTraversal traversal = PathTraversal::forward) noexcept;

[[nodiscard]] Polyline3d tessellate_path_geometry(
    const PathGeometry3d& path,
    const TessellationOptions& options = {});

[[nodiscard]] std::optional<BoundingBox3d> path_bounding_box(
    const PathGeometry3d& path,
    const TessellationOptions& options = {});

}  // namespace automap::core
