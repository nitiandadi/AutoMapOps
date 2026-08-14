#pragma once

#include "automap/core/map_id.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace automap::io {

// Options reserved for the Canonical JSON reader implemented in M3.
struct CanonicalJsonReadOptions {
    std::optional<core::MapId> expected_map_id;
    bool reject_unknown_fields{true};
};

// Options reserved for the Canonical JSON writer implemented in M3.
struct CanonicalJsonWriteOptions {
    bool pretty_print{true};
    std::uint8_t indent_spaces{2};
};

[[nodiscard]] std::string_view canonical_json_format_name() noexcept;
[[nodiscard]] std::string_view canonical_json_media_type() noexcept;
[[nodiscard]] std::string_view canonical_json_schema_version() noexcept;

}  // namespace automap::io
