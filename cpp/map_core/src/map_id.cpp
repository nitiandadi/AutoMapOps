#include "automap/core/map_id.hpp"

#include <utility>

namespace automap::core {

MapId::MapId(std::string value)
    : value_(std::move(value)) {}

const std::string& MapId::value() const noexcept {
    return value_;
}

bool MapId::empty() const noexcept {
    return value_.empty();
}

}  // namespace automap::core
