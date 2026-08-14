#pragma once

#include <string>

namespace automap::validation {

enum class Severity {
    warning,
    error,
    fatal
};

struct ValidationIssue {
    std::string rule_id;
    Severity severity{Severity::error};
    std::string object_id;
    std::string message;
    std::string suggestion;
};

}  // namespace automap::validation
