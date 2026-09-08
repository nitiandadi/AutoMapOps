#pragma once

#include "automap/core/map_data.hpp"
#include "automap/validation/validation_report.hpp"
#include "automap/version/version_id.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace automap::version {

struct PublishResult {
    bool published{false};
    core::MapId map_id;
    VersionId version_id;
    std::filesystem::path output_directory;
    std::string content_hash;
    std::string message;
};

struct PublishOptions final {
    std::filesystem::path output_directory;
    VersionId version_id{std::string{"V1"}};
    std::optional<VersionId> parent_version;
    std::string source_change_set_id;
    std::string description{"首个正式逻辑地图版本"};
    std::string created_by{"automap_cli"};
    std::string published_at_utc;
};

struct SnapshotVerificationResult final {
    bool valid{false};
    std::string content_hash;
    std::string message;
};

class VersionPublisher final {
public:
    [[nodiscard]] PublishResult evaluate(
        const core::MapId& map_id,
        const VersionId& version_id,
        const validation::ValidationReport& report) const;

    [[nodiscard]] PublishResult publish(
        const core::MapData& source_map,
        const validation::ValidationReport& report,
        const PublishOptions& options) const;
};

[[nodiscard]] SnapshotVerificationResult verify_map_snapshot(
    const std::filesystem::path& map_file,
    const core::MapId& expected_map_id,
    std::string_view expected_content_hash);

}  // namespace automap::version
