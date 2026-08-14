#include "automap/validation/validation_report.hpp"

#include <algorithm>
#include <utility>

namespace automap::validation {

ValidationReport::ValidationReport(core::MapId map_id)
    : map_id_(std::move(map_id)) {}

void ValidationReport::add_issue(ValidationIssue issue) {
    issues_.push_back(std::move(issue));
}

const core::MapId& ValidationReport::map_id() const noexcept {
    return map_id_;
}

const std::vector<ValidationIssue>& ValidationReport::issues() const noexcept {
    return issues_;
}

std::size_t ValidationReport::count(Severity severity) const noexcept {
    return static_cast<std::size_t>(std::count_if(
        issues_.begin(),
        issues_.end(),
        [severity](const ValidationIssue& issue) {
            return issue.severity == severity;
        }));
}

bool ValidationReport::can_publish() const noexcept {
    return count(Severity::fatal) == 0 && count(Severity::error) == 0;
}

}  // namespace automap::validation
