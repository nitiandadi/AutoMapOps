#include "automap/core/geometry.hpp"

#include <cmath>
#include <iostream>
#include <numbers>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) std::cerr << "失败：" << message << '\n';
    return condition;
}

bool close(double first, double second, double tolerance = 1e-6) {
    return std::abs(first - second) <= tolerance;
}

}  // namespace

int main() {
    using namespace automap::core;
    bool passed = true;

    const CurveSegment3d line = LineCurveSegment3d{
        .start = {1.0, 2.0, 3.0}, .heading_rad = 0.0,
        .length_m = 10.0, .end_z_m = 5.0};
    const auto line_middle = evaluate_curve_segment(line, 5.0);
    passed &= check(close(line_middle.position.x, 6.0) && close(line_middle.position.z, 4.0),
                    "直线位置和 Z 插值应正确。");

    const CurveSegment3d arc = CircularArcSegment3d{
        .start = {0.0, 0.0, 0.0}, .heading_rad = 0.0,
        .length_m = std::numbers::pi_v<double> * 5.0,
        .end_z_m = 1.0, .curvature_per_m = 0.1};
    const auto arc_end = evaluate_curve_segment(arc, curve_segment_length(arc));
    passed &= check(close(arc_end.position.x, 10.0) && close(arc_end.position.y, 10.0),
                    "正曲率圆弧端点应正确。");
    passed &= check(close(arc_end.heading_rad, std::numbers::pi_v<double> * 0.5),
                    "圆弧终点航向应正确。");

    const CurveSegment3d clothoid = ClothoidSegment3d{
        .start = {20.0, 0.0, 0.5}, .heading_rad = 0.0,
        .length_m = 10.0, .end_z_m = 1.0,
        .start_curvature_per_m = 0.0, .end_curvature_per_m = 0.05};
    const auto clothoid_end = evaluate_curve_segment(clothoid, 10.0);
    passed &= check(close(clothoid_end.position.x, 29.9376805843) &&
                        close(clothoid_end.position.y, 0.8296204854),
                    "Clothoid 黄金端点应与 TypeScript 数据一致。");
    passed &= check(close(clothoid_end.heading_rad, 0.25) &&
                        close(clothoid_end.curvature_per_m, 0.05),
                    "Clothoid 航向和终点曲率应正确。");

    const PathGeometry3d path{CompositeCurve3d{.segments = {line, clothoid}}};
    const auto reverse_start = path_start_state(path, PathTraversal::reverse);
    const auto forward_end = path_end_state(path);
    passed &= check(points_coincident(reverse_start.position, forward_end.position),
                    "反向遍历起点应等于正向终点。");
    passed &= check(close(
                        std::abs(normalize_heading(
                            reverse_start.heading_rad - forward_end.heading_rad)),
                        std::numbers::pi_v<double>),
                    "反向遍历航向应反转 π。");
    passed &= check(close(reverse_start.curvature_per_m, -forward_end.curvature_per_m),
                    "反向遍历曲率符号应反转。");

    const auto tessellated = tessellate_path_geometry(
        path, {.max_chord_error_m = 0.01, .max_segment_length_m = 1.0});
    passed &= check(tessellated.size() > 10U, "曲线路径应产生确定性细分点。");
    return passed ? 0 : 1;
}
