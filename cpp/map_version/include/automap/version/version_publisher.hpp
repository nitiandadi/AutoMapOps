#pragma once

#include "automap/core/map_id.hpp"
#include "automap/validation/validation_report.hpp"
#include "automap/version/version_id.hpp"

#include <string>

namespace automap::version {

struct PublishResult {
    bool published{false};
    core::MapId map_id;
    VersionId version_id;
    std::string message;
};

class VersionPublisher final {
public:
    [[nodiscard]] PublishResult evaluate(
        const core::MapId& map_id,
        const VersionId& version_id,
        const validation::ValidationReport& report) const;
};

}  // namespace automap::version
