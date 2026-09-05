#include "automap/io/canonical_json.hpp"

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "失败：" << message << '\n';
        return false;
    }
    return true;
}

template <typename Action>
bool check_error_path(Action action, std::string_view expected_path, std::string_view message) {
    try {
        action();
    } catch (const automap::io::CanonicalJsonError& error) {
        return check(error.json_path() == expected_path, message);
    }
    return check(false, message);
}

}  // namespace

int main() {
    using automap::core::MapId;
    using automap::io::CanonicalJsonReadOptions;
    using automap::io::CanonicalJsonWriteOptions;
    using automap::io::read_canonical_json;
    using automap::io::read_canonical_json_file;
    using automap::io::write_canonical_json;
    using automap::io::write_canonical_json_file;

    bool passed = true;
    const std::filesystem::path source_path =
        std::filesystem::path{AUTOMAP_SOURCE_DIR} / "maps" / "drafts" /
        "logistics_park_v0.json";

    const auto map = read_canonical_json_file(
        source_path,
        CanonicalJsonReadOptions{.expected_map_id = MapId{"logistics_park"}});

    passed &= check(map.header.name == "物流园 Canonical V0 草稿", "应完整读取中文地图名称。");
    passed &= check(map.roads.size() == 7U, "应读取 7 条 Road。");
    passed &= check(map.lanes.size() == 12U, "应读取 12 条 Lane。");
    passed &= check(map.lane_boundaries.size() == 19U, "应读取 19 条 LaneBoundary。");
    passed &= check(map.junctions.size() == 1U, "应读取 1 个 Junction。");
    passed &= check(map.lane_connections.size() == 3U, "应读取 3 个 LaneConnection。");
    passed &= check(map.operational_areas.size() == 4U, "应读取 4 个业务区域。");
    passed &= check(map.stations.size() == 4U, "应读取 4 个 Station。");
    passed &= check(map.restricted_areas.size() == 1U, "应读取 1 个限行区域。");
    passed &= check(map.vehicle_profiles.size() == 2U, "应读取 2 个车辆模型。");

    const std::string pretty_json = write_canonical_json(map);
    passed &= check(!pretty_json.empty() && pretty_json.back() == '\n', "格式化 JSON 应以换行结束。");
    passed &= check(read_canonical_json(pretty_json) == map, "格式化写出后读回应与原地图完全一致。");

    const std::string compact_json = write_canonical_json(
        map, CanonicalJsonWriteOptions{.pretty_print = false});
    passed &= check(compact_json.find('\n') == std::string::npos, "紧凑 JSON 不应包含排版换行。");
    passed &= check(read_canonical_json(compact_json) == map, "紧凑写出后读回应与原地图完全一致。");

    automap::core::MapData sparse_map;
    sparse_map.header.map_id = MapId{"sparse_map"};
    sparse_map.header.name = "含可选空值的地图";
    sparse_map.vehicle_profiles.push_back(automap::core::VehicleProfile{
        .id = "vehicle_optional",
        .name = "未提供长高的车辆",
        .type = automap::core::VehicleType::delivery_van,
        .width_m = 2.0,
    });
    const std::string sparse_json = write_canonical_json(sparse_map);
    passed &= check(
        sparse_json.find("\"heightM\": null") != std::string::npos,
        "未设置的车辆可选尺寸应写为 null。");
    passed &= check(
        read_canonical_json(sparse_json) == sparse_map,
        "包含 null 可选尺寸的地图应保持往返一致。");

    const std::filesystem::path round_trip_path =
        std::filesystem::temp_directory_path() / "automap_m3_canonical_round_trip.json";
    write_canonical_json_file(round_trip_path, map);
    passed &= check(read_canonical_json_file(round_trip_path) == map, "文件写出后应能完整读回。");
    std::error_code remove_error;
    std::filesystem::remove(round_trip_path, remove_error);

    std::string with_unknown_field = pretty_json;
    with_unknown_field.insert(with_unknown_field.find('{') + 1U, "\n  \"unknownField\": 1,");
    passed &= check_error_path(
        [&] { static_cast<void>(read_canonical_json(with_unknown_field)); },
        "$.unknownField",
        "默认应拒绝未知字段并返回准确 JSON 路径。");
    passed &= check(
        read_canonical_json(
            with_unknown_field,
            CanonicalJsonReadOptions{.reject_unknown_fields = false}) == map,
        "关闭未知字段拒绝选项后应忽略扩展字段。");

    passed &= check_error_path(
        [&] {
            static_cast<void>(read_canonical_json(
                pretty_json,
                CanonicalJsonReadOptions{.expected_map_id = MapId{"another_map"}}));
        },
        "$.header.mapId",
        "地图 ID 不符合预期时应返回准确 JSON 路径。");

    auto invalid_number_map = map;
    invalid_number_map.lanes[0].width_m = std::numeric_limits<double>::infinity();
    passed &= check_error_path(
        [&] { static_cast<void>(write_canonical_json(invalid_number_map)); },
        "$",
        "非有限浮点数不应被写入 JSON。");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M3 Canonical JSON 读写测试通过。\n";
    return 0;
}
