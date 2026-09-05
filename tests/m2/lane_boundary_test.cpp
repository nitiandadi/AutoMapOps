#include "automap/core/lane_boundary.hpp"
#include "automap/core/map_data.hpp"

#include <iostream>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "失败：" << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    using namespace automap::core;

    bool passed = true;

    const LaneBoundary shared_center{
        .id = "boundary_main_center",
        .geometry = {{0.0, 0.0, 0.0}, {20.0, 0.0, 0.0}, {50.0, 0.0, 0.0}},
        .type = LaneBoundaryType::dashed_line,
        .crossing_allowed = true,
    };
    const LaneBoundary outer_edge{
        .id = "boundary_main_right",
        .geometry = {{0.0, -3.5, 0.0}, {50.0, -3.5, 0.0}},
        .type = LaneBoundaryType::solid_line,
        .crossing_allowed = false,
    };
    const LaneBoundary curb{
        .id = "boundary_loading_curb",
        .geometry = {{0.0, -7.0, 0.0}, {50.0, -7.0, 0.0}},
        .type = LaneBoundaryType::curb,
        .crossing_allowed = false,
    };

    passed &= check(polyline_length(shared_center.geometry) == 50.0,
                    "LaneBoundary 应以米为单位保存几何。");
    const auto bounds = bounding_box(shared_center.geometry);
    passed &= check(bounds.has_value() &&
                        bounds->contains(Point3d{20.0, 0.0, 0.0}),
                    "LaneBoundary 几何应能使用公共几何算法。");
    passed &= check(lane_boundary_type_name(shared_center.type) == "dashed_line",
                    "边界类型应具有稳定的序列化名称。");
    passed &= check(lane_boundary_type_name(LaneBoundaryType::double_solid_line) ==
                        "double_solid_line",
                    "双实线应具有稳定的序列化名称。");
    passed &= check(lane_boundary_type_name(LaneBoundaryType::virtual_boundary) ==
                        "virtual_boundary",
                    "逻辑边界应能被明确表达。");
    passed &= check(shared_center.crossing_allowed && !outer_edge.crossing_allowed &&
                        !curb.crossing_allowed,
                    "跨越权限应独立且明确地表达。");

    MapData map;
    map.lane_boundaries = {shared_center, outer_edge, curb};

    LaneBoundary* mutable_boundary = map.find_lane_boundary("boundary_main_center");
    passed &= check(mutable_boundary != nullptr,
                    "MapData 应能通过稳定 ID 查找可修改的 LaneBoundary。");
    if (mutable_boundary != nullptr) {
        passed &= check(*mutable_boundary == shared_center,
                        "LaneBoundary 应支持用于往返测试的值比较。");
        mutable_boundary->crossing_allowed = false;
    }

    const MapData& const_map = map;
    const LaneBoundary* found_boundary =
        const_map.find_lane_boundary("boundary_main_center");
    passed &= check(found_boundary != nullptr && !found_boundary->crossing_allowed,
                    "只读查询应能观察到边界更新。");
    passed &= check(const_map.find_lane_boundary("boundary_missing") == nullptr,
                    "查找不存在的 LaneBoundary 应返回 nullptr。");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M2 LaneBoundary 测试通过。\n";
    return 0;
}
