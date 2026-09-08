#include "automap/validation/map_validation.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool check(bool condition, std::string_view message) {
    if (!condition) std::cerr << "失败：" << message << '\n';
    return condition;
}

}  // namespace

int main() {
    using namespace automap::validation;

    ValidationReport report{automap::core::MapId{"report_demo"}};
    report.add_issue({
        .rule_id = "DEMO_WARNING",
        .severity = Severity::warning,
        .object_id = "lane_\"demo\"",
        .message = "示例原因\n第二行",
        .suggestion = "检查车道。",
    });
    report.add_issue({
        .rule_id = "DEMO_ERROR",
        .severity = Severity::error,
        .object_id = "road_demo",
        .message = "示例错误。",
        .suggestion = "修正道路。",
    });

    const std::string json = write_validation_report_json(report);
    bool passed = true;
    passed &= check(json.find("\"mapId\": \"report_demo\"") != std::string::npos,
                    "报告应包含地图 ID。");
    passed &= check(json.find("\"warning\": 1") != std::string::npos,
                    "报告应统计 Warning。");
    passed &= check(json.find("\"error\": 1") != std::string::npos,
                    "报告应统计 Error。");
    passed &= check(json.find("\"fatal\": 0") != std::string::npos,
                    "报告应统计 Fatal。");
    passed &= check(json.find("\"canPublish\": false") != std::string::npos,
                    "存在 Error 时报告应标记为不可发布。");
    passed &= check(json.find("\"severity\": \"warning\"") != std::string::npos,
                    "问题应包含等级。");
    passed &= check(json.find("lane_\\\"demo\\\"") != std::string::npos,
                    "对象 ID 应进行 JSON 转义。");
    passed &= check(json.find("示例原因\\n第二行") != std::string::npos,
                    "原因中的换行应进行 JSON 转义。");
    passed &= check(json.find("\"suggestion\": \"检查车道。\"") != std::string::npos,
                    "问题应包含修复建议。");

    const std::filesystem::path report_path =
        std::filesystem::temp_directory_path() / "automap_validation_report_test.json";
    write_validation_report_json_file(report_path, report);
    std::ifstream input(report_path, std::ios::binary);
    std::ostringstream saved;
    saved << input.rdbuf();
    passed &= check(saved.str() == json, "文件报告内容应与字符串写出结果一致。");
    std::error_code ignored;
    std::filesystem::remove(report_path, ignored);

    if (!passed) return 1;
    std::cout << "AutoMapOps M3 质检报告 JSON 测试通过。\n";
    return 0;
}
