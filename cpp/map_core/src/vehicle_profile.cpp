#include "automap/core/vehicle_profile.hpp"

namespace automap::core {

std::string_view vehicle_type_name(VehicleType type) noexcept {
    switch (type) {
        case VehicleType::passenger_car:
            return "passenger_car";
        case VehicleType::delivery_van:
            return "delivery_van";
        case VehicleType::truck:
            return "truck";
    }

    return "passenger_car";
}

}  // namespace automap::core
