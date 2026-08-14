#include "automap/io/canonical_json.hpp"

namespace automap::io {

std::string_view canonical_json_format_name() noexcept {
    return "AutoMapOps Canonical JSON";
}

std::string_view canonical_json_media_type() noexcept {
    return "application/json";
}

std::string_view canonical_json_schema_version() noexcept {
    return "1.0";
}

}  // namespace automap::io
