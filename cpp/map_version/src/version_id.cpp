#include "automap/version/version_id.hpp"

#include <utility>

namespace automap::version {

VersionId::VersionId(std::string value)
    : value_(std::move(value)) {}

const std::string& VersionId::value() const noexcept {
    return value_;
}

bool VersionId::empty() const noexcept {
    return value_.empty();
}

}  // namespace automap::version
