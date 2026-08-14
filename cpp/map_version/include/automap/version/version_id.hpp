#pragma once

#include <compare>
#include <string>

namespace automap::version {

class VersionId final {
public:
    VersionId() = default;
    explicit VersionId(std::string value);

    [[nodiscard]] const std::string& value() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    auto operator<=>(const VersionId&) const = default;

private:
    std::string value_;
};

}  // namespace automap::version
