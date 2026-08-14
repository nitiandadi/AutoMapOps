#pragma once

#include "automap/core/map_id.hpp"
#include "automap/validation/validation_issue.hpp"

#include <cstddef>
#include <vector>

namespace automap::validation {

class ValidationReport final {
public:
    explicit ValidationReport(core::MapId map_id);

    void add_issue(ValidationIssue issue);

    [[nodiscard]] const core::MapId& map_id() const noexcept;
    [[nodiscard]] const std::vector<ValidationIssue>& issues() const noexcept;
    [[nodiscard]] std::size_t count(Severity severity) const noexcept;
    [[nodiscard]] bool can_publish() const noexcept;

private:
    core::MapId map_id_;
    std::vector<ValidationIssue> issues_;
};

}  // namespace automap::validation
