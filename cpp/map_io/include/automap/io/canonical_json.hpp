#pragma once

#include "automap/core/map_data.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace automap::io {

struct CanonicalJsonReadOptions {
    std::optional<core::MapId> expected_map_id;
    bool reject_unknown_fields{true};
};

enum class CanonicalJsonSchemaVersion {
    v1_0,
    v1_1,
};

struct CanonicalJsonWriteOptions {
    bool pretty_print{true};
    std::uint8_t indent_spaces{2};
    std::optional<CanonicalJsonSchemaVersion> target_schema_version;
};

class CanonicalJsonError : public std::runtime_error {
public:
    CanonicalJsonError(std::string json_path, std::string message);

    [[nodiscard]] const std::string& json_path() const noexcept;

private:
    std::string json_path_;
};

[[nodiscard]] core::MapData read_canonical_json(
    std::string_view json,
    const CanonicalJsonReadOptions& options = {});

[[nodiscard]] core::MapData read_canonical_json_file(
    const std::filesystem::path& path,
    const CanonicalJsonReadOptions& options = {});

[[nodiscard]] std::string write_canonical_json(
    const core::MapData& map,
    const CanonicalJsonWriteOptions& options = {});

void write_canonical_json_file(
    const std::filesystem::path& path,
    const core::MapData& map,
    const CanonicalJsonWriteOptions& options = {});

[[nodiscard]] std::string_view canonical_json_format_name() noexcept;
[[nodiscard]] std::string_view canonical_json_media_type() noexcept;
[[nodiscard]] std::string_view canonical_json_schema_version() noexcept;

}  // namespace automap::io
