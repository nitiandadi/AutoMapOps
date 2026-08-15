#include "automap/core/scene_objects.hpp"

namespace automap::core {

std::string_view operational_area_type_name(OperationalAreaType type) noexcept {
    switch (type) {
        case OperationalAreaType::unknown:
            return "unknown";
        case OperationalAreaType::warehouse:
            return "warehouse";
        case OperationalAreaType::loading_area:
            return "loading_area";
        case OperationalAreaType::parking_area:
            return "parking_area";
        case OperationalAreaType::charging_area:
            return "charging_area";
    }

    return "unknown";
}

std::string_view station_type_name(StationType type) noexcept {
    switch (type) {
        case StationType::unknown:
            return "unknown";
        case StationType::gate:
            return "gate";
        case StationType::loading_bay:
            return "loading_bay";
        case StationType::parking_space:
            return "parking_space";
        case StationType::charging_point:
            return "charging_point";
        case StationType::waypoint:
            return "waypoint";
    }

    return "unknown";
}

}  // namespace automap::core
