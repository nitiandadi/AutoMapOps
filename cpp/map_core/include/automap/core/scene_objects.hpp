#pragma once

#include "automap/core/geometry.hpp"
#include "automap/core/object_id.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace automap::core {

// 园区内具有明确业务用途的一块区域，而不是车辆需要精确到达的目标点。
enum class OperationalAreaType {
    unknown,        // 未知或尚未完成分类的区域
    warehouse,      // 仓库及其相关作业区域
    loading_area,   // 货物装卸、月台作业区域
    parking_area,   // 车辆停车或待命区域
    charging_area,  // 车辆集中充电区域
};

// 区域内车辆可以精确到达的业务目标点，通过 access_lane_id 关联接入车道。
enum class StationType {
    unknown,         // 未知或尚未完成分类的目标点
    gate,            // 园区入口、出口或门岗目标点
    loading_bay,     // 具体装货或卸货月台位置
    parking_space,   // 具体停车位或车辆待命位置
    charging_point,  // 具体充电桩或充电车位位置
    waypoint,        // 不对应固定设施的通用途经点
};

[[nodiscard]] std::string_view operational_area_type_name(
    OperationalAreaType type) noexcept;
[[nodiscard]] std::string_view station_type_name(StationType type) noexcept;

struct OperationalArea final {
    ObjectId id;
    std::string name;
    OperationalAreaType type{OperationalAreaType::unknown};
    Polyline3d outline;

    bool operator==(const OperationalArea&) const = default;
};

struct Station final {
    ObjectId id;
    std::string name;
    StationType type{StationType::unknown};
    Point3d position;
    ObjectId access_lane_id;

    bool operator==(const Station&) const = default;
};

struct RestrictedArea final {
    ObjectId id;
    std::string name;
    Polyline3d outline;
    std::vector<ObjectId> allowed_vehicle_profile_ids;

    bool operator==(const RestrictedArea&) const = default;
};

}  // namespace automap::core
