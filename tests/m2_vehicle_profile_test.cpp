#include "automap/core/map_data.hpp"
#include "automap/core/vehicle_profile.hpp"

#include <iostream>
#include <string_view>

namespace {
bool check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}
}  // namespace

int main() {
    using namespace automap::core;
    bool passed = true;

    const VehicleProfile truck{
        .id = "vehicle_truck_12m",
        .name = "12 m Logistics Truck",
        .type = VehicleType::truck,
        .width_m = 2.55,
        .height_m = 4.0,
        .length_m = 12.0,
        .minimum_turning_radius_m = 10.5,
    };

    passed &= check(vehicle_type_name(truck.type) == "truck",
                    "Vehicle type should have a stable serialized name.");
    passed &= check(truck.width_m == 2.55 && truck.height_m == 4.0,
                    "Vehicle width and optional height should use metres.");
    passed &= check(truck.length_m == 12.0 && truck.minimum_turning_radius_m == 10.5,
                    "Basic passage constraints should be retained.");

    MapData map;
    map.vehicle_profiles.push_back(truck);
    VehicleProfile* found = map.find_vehicle_profile(truck.id);
    passed &= check(found != nullptr && *found == truck,
                    "MapData should find and compare a vehicle profile.");
    passed &= check(map.find_vehicle_profile("vehicle_missing") == nullptr,
                    "Missing vehicle lookup should return nullptr.");

    if (!passed) {
        return 1;
    }
    std::cout << "AutoMapOps M2 vehicle profile test passed.\n";
    return 0;
}
