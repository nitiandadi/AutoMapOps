#include "automap/version/version_publisher.hpp"

namespace automap::version {

PublishResult VersionPublisher::evaluate(
    const core::MapId& map_id,
    const VersionId& version_id,
    const validation::ValidationReport& report) const {
    if (map_id.empty() || version_id.empty()) {
        return {false, map_id, version_id, "Map ID and version ID are required."};
    }

    if (report.map_id() != map_id) {
        return {false, map_id, version_id, "Validation report belongs to another map."};
    }

    if (!report.can_publish()) {
        return {false, map_id, version_id, "Validation errors block publication."};
    }

    return {true, map_id, version_id, "Version is eligible for publication."};
}

}  // namespace automap::version
