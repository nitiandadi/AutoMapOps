#pragma once

#include <compare>
#include <string>

namespace automap::core {

// Stable identifier of one map project, for example "logistics_park_demo".
// Format and non-empty validation belong to the validation module, not here.
class MapId final {
public:
    MapId() = default;
    explicit MapId(std::string value);

    [[nodiscard]] const std::string& value() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    auto operator<=>(const MapId&) const = default;

private:
    std::string value_;
};

}  // namespace automap::core
