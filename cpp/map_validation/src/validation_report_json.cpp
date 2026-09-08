#include "automap/validation/validation_report_json.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace automap::validation {
namespace {

[[nodiscard]] std::string quote_json(std::string_view value) {
    std::ostringstream output;
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20U) {
                    output << "\\u" << std::hex << std::setw(4)
                           << std::setfill('0') << static_cast<unsigned>(character)
                           << std::dec << std::setfill(' ');
                } else {
                    output << static_cast<char>(character);
                }
        }
    }
    output << '"';
    return output.str();
}

}  // namespace

std::string_view severity_name(Severity severity) noexcept {
    switch (severity) {
        case Severity::warning: return "warning";
        case Severity::error: return "error";
        case Severity::fatal: return "fatal";
    }
    return "error";
}

std::string write_validation_report_json(
    const ValidationReport& report,
    bool pretty_print) {
    const std::string newline = pretty_print ? "\n" : "";
    const std::string indent1 = pretty_print ? "  " : "";
    const std::string indent2 = pretty_print ? "    " : "";
    const std::string indent3 = pretty_print ? "      " : "";
    const std::string separator = pretty_print ? ": " : ":";

    std::ostringstream output;
    output << '{' << newline
           << indent1 << "\"schemaVersion\"" << separator << "\"1.0\"," << newline
           << indent1 << "\"mapId\"" << separator
           << quote_json(report.map_id().value()) << ',' << newline
           << indent1 << "\"summary\"" << separator << '{' << newline
           << indent2 << "\"warning\"" << separator
           << report.count(Severity::warning) << ',' << newline
           << indent2 << "\"error\"" << separator
           << report.count(Severity::error) << ',' << newline
           << indent2 << "\"fatal\"" << separator
           << report.count(Severity::fatal) << ',' << newline
           << indent2 << "\"canPublish\"" << separator
           << (report.can_publish() ? "true" : "false") << newline
           << indent1 << "}," << newline
           << indent1 << "\"issues\"" << separator << '[';

    const auto& issues = report.issues();
    if (!issues.empty()) output << newline;
    for (std::size_t index = 0U; index < issues.size(); ++index) {
        const auto& issue = issues[index];
        output << indent2 << '{' << newline
               << indent3 << "\"ruleId\"" << separator
               << quote_json(issue.rule_id) << ',' << newline
               << indent3 << "\"severity\"" << separator
               << quote_json(severity_name(issue.severity)) << ',' << newline
               << indent3 << "\"objectId\"" << separator
               << quote_json(issue.object_id) << ',' << newline
               << indent3 << "\"reason\"" << separator
               << quote_json(issue.message) << ',' << newline
               << indent3 << "\"suggestion\"" << separator
               << quote_json(issue.suggestion) << newline
               << indent2 << '}';
        if (index + 1U != issues.size()) output << ',';
        output << newline;
    }
    output << (issues.empty() ? "" : indent1) << ']' << newline << '}' << newline;
    return output.str();
}

void write_validation_report_json_file(
    const std::filesystem::path& path,
    const ValidationReport& report,
    bool pretty_print) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("无法写入质检报告：" + path.string());
    }
    output << write_validation_report_json(report, pretty_print);
    if (!output) {
        throw std::runtime_error("写入质检报告失败：" + path.string());
    }
}

}  // namespace automap::validation
