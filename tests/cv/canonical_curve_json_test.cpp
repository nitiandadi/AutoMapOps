#include "automap/io/canonical_json.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) std::cerr << "失败：" << message << '\n';
    return condition;
}

}  // namespace

int main() {
    using namespace automap;
    bool passed = true;
    const auto source = std::filesystem::path{AUTOMAP_SOURCE_DIR} /
                        "maps/drafts/canonical_curve_demo_v1_1.json";
    const core::MapData map = io::read_canonical_json_file(source);
    passed &= check(map.header.schema_version == "1.1", "应读取 Canonical 1.1。");
    passed &= check(map.roads.size() == 3U && map.roads[0].reference_line.is_composite_curve(),
                    "Road referenceLine 应保留组合曲线表示。");
    passed &= check(map.roads[0].reference_line.composite_curve()->segments.size() == 4U,
                    "曲线示例主路应保留四个曲线段。");

    const std::string serialized = io::write_canonical_json(map);
    const core::MapData roundtrip = io::read_canonical_json(serialized);
    passed &= check(roundtrip == map, "Canonical 1.1 曲线读写往返后应值一致。");

    bool rejected_v1 = false;
    try {
        static_cast<void>(io::write_canonical_json(
            map, {.target_schema_version = io::CanonicalJsonSchemaVersion::v1_0}));
    } catch (const io::CanonicalJsonError& error) {
        rejected_v1 = error.json_path() == "$.roads[0].referenceLine";
    }
    passed &= check(rejected_v1, "曲线写入 1.0 时应报告准确 JSON Path。");
    return passed ? 0 : 1;
}
