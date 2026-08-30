#include "automap/core/map_id.hpp"
#include "automap/io/canonical_json.hpp"
#include "automap/validation/validation_issue.hpp"
#include "automap/validation/validation_report.hpp"
#include "automap/version/version_id.hpp"
#include "automap/version/version_publisher.hpp"

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
    using automap::core::MapId;
    using automap::validation::Severity;
    using automap::validation::ValidationIssue;
    using automap::validation::ValidationReport;
    using automap::version::VersionId;
    using automap::version::VersionPublisher;

    bool passed = true;
    const MapId map_id{"logistics_park_demo"};

    passed &= check(!map_id.empty(), "MapId 应保存非空值。");
    passed &= check(
        automap::io::canonical_json_schema_version() == "1.0",
        "Canonical JSON Schema 版本应为 1.0。");

    ValidationReport clean_report{map_id};
    const VersionPublisher publisher;
    const auto accepted = publisher.evaluate(map_id, VersionId{"V1"}, clean_report);
    passed &= check(accepted.published, "无问题的质检报告应允许发布。");

    ValidationReport blocked_report{map_id};
    blocked_report.add_issue(ValidationIssue{
        .rule_id = "SMOKE_ERROR",
        .severity = Severity::error,
        .object_id = "lane_demo",
        .message = "M1 冒烟测试使用的模拟错误。",
        .suggestion = "移除模拟错误。",
    });

    const auto rejected = publisher.evaluate(map_id, VersionId{"V1"}, blocked_report);
    passed &= check(!rejected.published, "存在错误时应阻止发布。");
    passed &= check(
        blocked_report.count(Severity::error) == 1,
        "ValidationReport 应统计到一个错误。");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M1 冒烟测试通过。\n";
    return 0;
}
