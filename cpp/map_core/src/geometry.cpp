#include "automap/core/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <type_traits>
#include <utility>

namespace automap::core {
namespace {

[[nodiscard]] double horizontal_distance(
    const Point3d& first,
    const Point3d& second) noexcept {
    return std::hypot(second.x - first.x, second.y - first.y);
}

template <typename Function>
[[nodiscard]] double adaptive_simpson(
    const Function& function,
    double start,
    double end,
    double tolerance,
    std::size_t depth) noexcept {
    const auto simpson = [&](double left, double right) {
        const double middle = (left + right) * 0.5;
        return (right - left) / 6.0 *
               (function(left) + 4.0 * function(middle) + function(right));
    };

    const double whole = simpson(start, end);
    const auto recurse = [&](auto&& self, double left, double right,
                             double expected, double epsilon,
                             std::size_t remaining) -> double {
        const double middle = (left + right) * 0.5;
        const double first = simpson(left, middle);
        const double second = simpson(middle, right);
        const double delta = first + second - expected;
        if (remaining == 0U || std::abs(delta) <= 15.0 * epsilon) {
            return first + second + delta / 15.0;
        }
        return self(self, left, middle, first, epsilon * 0.5, remaining - 1U) +
               self(self, middle, right, second, epsilon * 0.5, remaining - 1U);
    };
    return recurse(recurse, start, end, whole, tolerance, depth);
}

[[nodiscard]] CurveState3d evaluate_forward_segment(
    const CurveSegment3d& segment,
    double requested_s) noexcept {
    return std::visit(
        [requested_s](const auto& value) -> CurveState3d {
            const double length = std::max(0.0, value.length_m);
            const double s = std::clamp(requested_s, 0.0, length);
            const double z_ratio = length > 0.0 ? s / length : 0.0;
            const double z = value.start.z + (value.end_z_m - value.start.z) * z_ratio;

            using Segment = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Segment, LineCurveSegment3d>) {
                return {
                    .position = {
                        value.start.x + std::cos(value.heading_rad) * s,
                        value.start.y + std::sin(value.heading_rad) * s,
                        z,
                    },
                    .heading_rad = normalize_heading(value.heading_rad),
                    .curvature_per_m = 0.0,
                };
            } else if constexpr (std::is_same_v<Segment, CircularArcSegment3d>) {
                const double curvature = value.curvature_per_m;
                if (std::abs(curvature) <= tolerance::floating_point) {
                    return {
                        .position = {
                            value.start.x + std::cos(value.heading_rad) * s,
                            value.start.y + std::sin(value.heading_rad) * s,
                            z,
                        },
                        .heading_rad = normalize_heading(value.heading_rad),
                        .curvature_per_m = curvature,
                    };
                }
                const double heading = value.heading_rad + curvature * s;
                return {
                    .position = {
                        value.start.x +
                            (std::sin(heading) - std::sin(value.heading_rad)) / curvature,
                        value.start.y -
                            (std::cos(heading) - std::cos(value.heading_rad)) / curvature,
                        z,
                    },
                    .heading_rad = normalize_heading(heading),
                    .curvature_per_m = curvature,
                };
            } else {
                const double curvature_rate = length > 0.0
                    ? (value.end_curvature_per_m - value.start_curvature_per_m) / length
                    : 0.0;
                const auto heading_at = [&](double distance_m) {
                    return value.heading_rad + value.start_curvature_per_m * distance_m +
                           0.5 * curvature_rate * distance_m * distance_m;
                };
                const double integration_tolerance =
                    std::max(1e-12, std::abs(s) * 1e-12);
                const double offset_x = s == 0.0 ? 0.0 : adaptive_simpson(
                    [&](double distance_m) { return std::cos(heading_at(distance_m)); },
                    0.0, s, integration_tolerance, 18U);
                const double offset_y = s == 0.0 ? 0.0 : adaptive_simpson(
                    [&](double distance_m) { return std::sin(heading_at(distance_m)); },
                    0.0, s, integration_tolerance, 18U);
                return {
                    .position = {value.start.x + offset_x, value.start.y + offset_y, z},
                    .heading_rad = normalize_heading(heading_at(s)),
                    .curvature_per_m = value.start_curvature_per_m + curvature_rate * s,
                };
            }
        },
        segment);
}

void append_tessellated_interval(
    const CurveSegment3d& segment,
    double start_s,
    const CurveState3d& start_state,
    double end_s,
    const CurveState3d& end_state,
    const TessellationOptions& options,
    std::size_t depth,
    Polyline3d& result) {
    const double middle_s = (start_s + end_s) * 0.5;
    const CurveState3d middle_state = evaluate_curve_segment(segment, middle_s);
    const Point3d chord_middle{
        (start_state.position.x + end_state.position.x) * 0.5,
        (start_state.position.y + end_state.position.y) * 0.5,
        (start_state.position.z + end_state.position.z) * 0.5,
    };
    const double chord_error = horizontal_distance(middle_state.position, chord_middle);
    const double interval_length = end_s - start_s;
    const bool subdivide = depth < options.max_recursion_depth &&
        (chord_error > options.max_chord_error_m ||
         interval_length > options.max_segment_length_m);
    if (subdivide) {
        append_tessellated_interval(
            segment, start_s, start_state, middle_s, middle_state,
            options, depth + 1U, result);
        append_tessellated_interval(
            segment, middle_s, middle_state, end_s, end_state,
            options, depth + 1U, result);
        return;
    }
    result.push_back(end_state.position);
}

}  // namespace

PathGeometry3d::PathGeometry3d(Polyline3d polyline)
    : storage_(std::move(polyline)) {}

PathGeometry3d::PathGeometry3d(std::initializer_list<Point3d> points)
    : storage_(Polyline3d{points}) {}

PathGeometry3d::PathGeometry3d(CompositeCurve3d curve)
    : storage_(std::move(curve)) {}

bool PathGeometry3d::is_polyline() const noexcept {
    return std::holds_alternative<Polyline3d>(storage_);
}

bool PathGeometry3d::is_composite_curve() const noexcept {
    return std::holds_alternative<CompositeCurve3d>(storage_);
}

const Polyline3d* PathGeometry3d::polyline() const noexcept {
    return std::get_if<Polyline3d>(&storage_);
}

Polyline3d* PathGeometry3d::polyline() noexcept {
    return std::get_if<Polyline3d>(&storage_);
}

const CompositeCurve3d* PathGeometry3d::composite_curve() const noexcept {
    return std::get_if<CompositeCurve3d>(&storage_);
}

CompositeCurve3d* PathGeometry3d::composite_curve() noexcept {
    return std::get_if<CompositeCurve3d>(&storage_);
}

const PathGeometry3d::Storage& PathGeometry3d::storage() const noexcept {
    return storage_;
}

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

double polyline_length(const PathGeometry3d& path) noexcept {
    return path_planar_length(path);
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

std::optional<BoundingBox3d> bounding_box(const PathGeometry3d& path) {
    return path_bounding_box(path);
}

std::optional<BoundingBox3d> bounding_box(
    std::initializer_list<Point3d> points) noexcept {
    return bounding_box(Polyline3d{points});
}

double normalize_heading(double heading_rad) noexcept {
    return std::remainder(heading_rad, 2.0 * std::numbers::pi_v<double>);
}

double curve_segment_length(const CurveSegment3d& segment) noexcept {
    return std::visit([](const auto& value) { return value.length_m; }, segment);
}

CurveState3d evaluate_curve_segment(
    const CurveSegment3d& segment,
    double s) noexcept {
    return evaluate_forward_segment(segment, s);
}

double path_planar_length(const PathGeometry3d& path) noexcept {
    if (const Polyline3d* polyline = path.polyline()) {
        double length = 0.0;
        for (std::size_t index = 1U; index < polyline->size(); ++index) {
            length += horizontal_distance((*polyline)[index - 1U], (*polyline)[index]);
        }
        return length;
    }

    double length = 0.0;
    for (const CurveSegment3d& segment : path.composite_curve()->segments) {
        const double segment_length = curve_segment_length(segment);
        if (std::isfinite(segment_length) && segment_length > 0.0) {
            length += segment_length;
        }
    }
    return length;
}

CurveState3d evaluate_path_geometry(
    const PathGeometry3d& path,
    double requested_s,
    PathTraversal traversal) noexcept {
    const double total_length = path_planar_length(path);
    const double traversal_s = std::clamp(requested_s, 0.0, total_length);
    const double forward_s = traversal == PathTraversal::forward
        ? traversal_s
        : total_length - traversal_s;

    CurveState3d state{};
    if (const Polyline3d* polyline = path.polyline()) {
        if (polyline->empty()) {
            return state;
        }
        state.position = polyline->front();
        double accumulated = 0.0;
        for (std::size_t index = 1U; index < polyline->size(); ++index) {
            const Point3d& first = (*polyline)[index - 1U];
            const Point3d& second = (*polyline)[index];
            const double segment_length = horizontal_distance(first, second);
            if (segment_length <= tolerance::floating_point) {
                continue;
            }
            if (forward_s <= accumulated + segment_length || index + 1U == polyline->size()) {
                const double local_s = std::clamp(forward_s - accumulated, 0.0, segment_length);
                const double ratio = local_s / segment_length;
                state.position = {
                    first.x + (second.x - first.x) * ratio,
                    first.y + (second.y - first.y) * ratio,
                    first.z + (second.z - first.z) * ratio,
                };
                state.heading_rad = std::atan2(second.y - first.y, second.x - first.x);
                break;
            }
            accumulated += segment_length;
        }
    } else {
        const auto& segments = path.composite_curve()->segments;
        if (segments.empty()) {
            return state;
        }
        double accumulated = 0.0;
        state = evaluate_curve_segment(segments.front(), 0.0);
        for (std::size_t index = 0U; index < segments.size(); ++index) {
            const double segment_length = std::max(0.0, curve_segment_length(segments[index]));
            if (forward_s <= accumulated + segment_length || index + 1U == segments.size()) {
                state = evaluate_curve_segment(
                    segments[index], std::clamp(forward_s - accumulated, 0.0, segment_length));
                break;
            }
            accumulated += segment_length;
        }
    }

    if (traversal == PathTraversal::reverse) {
        state.heading_rad = normalize_heading(state.heading_rad + std::numbers::pi_v<double>);
        state.curvature_per_m = -state.curvature_per_m;
    }
    return state;
}

CurveState3d path_start_state(
    const PathGeometry3d& path,
    PathTraversal traversal) noexcept {
    return evaluate_path_geometry(path, 0.0, traversal);
}

CurveState3d path_end_state(
    const PathGeometry3d& path,
    PathTraversal traversal) noexcept {
    return evaluate_path_geometry(path, path_planar_length(path), traversal);
}

Polyline3d tessellate_path_geometry(
    const PathGeometry3d& path,
    const TessellationOptions& requested_options) {
    if (const Polyline3d* polyline = path.polyline()) {
        return *polyline;
    }

    TessellationOptions options = requested_options;
    options.max_chord_error_m = std::max(options.max_chord_error_m, 1e-9);
    options.max_segment_length_m = std::max(options.max_segment_length_m, 1e-6);

    Polyline3d result;
    const auto& segments = path.composite_curve()->segments;
    for (const CurveSegment3d& segment : segments) {
        const double length = curve_segment_length(segment);
        const CurveState3d start = evaluate_curve_segment(segment, 0.0);
        if (result.empty() || !points_coincident(result.back(), start.position)) {
            result.push_back(start.position);
        }
        if (!std::isfinite(length) || length <= 0.0) {
            continue;
        }
        const CurveState3d end = evaluate_curve_segment(segment, length);
        append_tessellated_interval(
            segment, 0.0, start, length, end, options, 0U, result);
    }
    return result;
}

std::optional<BoundingBox3d> path_bounding_box(
    const PathGeometry3d& path,
    const TessellationOptions& options) {
    return bounding_box(tessellate_path_geometry(path, options));
}

}  // namespace automap::core
