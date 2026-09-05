#include "automap/validation/map_validation.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) std::cerr << "失败：" << message << '\n';
    return condition;
}

}  // namespace

int main() {
    using namespace automap;
    core::MapData map;
    map.header.map_id = core::MapId{"curve_validation"};
    map.roads.push_back({
        .id = "road_discontinuous_curve",
        .reference_line = core::CompositeCurve3d{.segments = {
            core::LineCurveSegment3d{
                .start = {0.0, 0.0, 0.0}, .heading_rad = 0.0,
                .length_m = 10.0, .end_z_m = 0.0},
            core::CircularArcSegment3d{
                .start = {11.0, 0.0, 0.0}, .heading_rad = 0.1,
                .length_m = 10.0, .end_z_m = 0.0,
                .curvature_per_m = 0.05},
        }},
    });

    validation::ValidationReport report{map.header.map_id};
    const validation::BasicGeometryRule rule;
    rule.validate(validation::ValidationContext{.map = map}, report);

    bool found_g0 = false;
    bool found_g1 = false;
    bool found_g2 = false;
    for (const auto& issue : report.issues()) {
        found_g0 |= issue.message.find("G0") != std::string::npos &&
                    issue.severity == validation::Severity::error;
        found_g1 |= issue.message.find("G1") != std::string::npos &&
                    issue.severity == validation::Severity::error;
        found_g2 |= issue.message.find("G2") != std::string::npos &&
                    issue.severity == validation::Severity::warning;
    }
    bool passed = true;
    passed &= check(found_g0, "曲线段端点间隙应产生 G0 Error。");
    passed &= check(found_g1, "曲线段航向突变应产生 G1 Error。");
    passed &= check(found_g2, "曲线段曲率突变应产生 G2 Warning。");
    return passed ? 0 : 1;
}
