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
        std::cerr << "FAILED: " << message << '\n';
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

    passed &= check(!map_id.empty(), "MapId should keep a non-empty value.");
    passed &= check(
        automap::io::canonical_json_schema_version() == "1.0",
        "Canonical JSON schema version should be 1.0.");

    ValidationReport clean_report{map_id};
    const VersionPublisher publisher;
    const auto accepted = publisher.evaluate(map_id, VersionId{"V1"}, clean_report);
    passed &= check(accepted.published, "A clean report should allow publication.");

    ValidationReport blocked_report{map_id};
    blocked_report.add_issue(ValidationIssue{
        .rule_id = "SMOKE_ERROR",
        .severity = Severity::error,
        .object_id = "lane_demo",
        .message = "Synthetic error for the M1 smoke test.",
        .suggestion = "Remove the synthetic error.",
    });

    const auto rejected = publisher.evaluate(map_id, VersionId{"V1"}, blocked_report);
    passed &= check(!rejected.published, "An error should block publication.");
    passed &= check(
        blocked_report.count(Severity::error) == 1,
        "ValidationReport should count one error.");

    if (!passed) {
        return 1;
    }

    std::cout << "AutoMapOps M1 smoke test passed.\n";
    return 0;
}
