#include "automap/core/junction.hpp"

namespace automap::core {

std::string_view turn_direction_name(TurnDirection direction) noexcept {
    switch (direction) {
        case TurnDirection::straight:
            return "straight";
        case TurnDirection::left:
            return "left";
        case TurnDirection::right:
            return "right";
        case TurnDirection::u_turn:
            return "u_turn";
    }

    return "straight";
}

}  // namespace automap::core
