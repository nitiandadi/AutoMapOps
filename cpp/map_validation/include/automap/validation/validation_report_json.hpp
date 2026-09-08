#pragma once

#include "automap/validation/validation_report.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace automap::validation {

[[nodiscard]] std::string_view severity_name(Severity severity) noexcept;

[[nodiscard]] std::string write_validation_report_json(
    const ValidationReport& report,
    bool pretty_print = true);

void write_validation_report_json_file(
    const std::filesystem::path& path,
    const ValidationReport& report,
    bool pretty_print = true);

}  // namespace automap::validation
