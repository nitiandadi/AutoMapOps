#include "automap/io/canonical_json.hpp"

#include "json_value.hpp"

#include "automap/core/map_header.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

namespace automap::io {
namespace {

using detail::JsonValue;
using JsonArray = JsonValue::Array;
using JsonObject = JsonValue::Object;

[[nodiscard]] std::string child_path(std::string_view path, std::string_view field) {
    return std::string(path) + "." + std::string(field);
}

[[nodiscard]] std::string index_path(std::string_view path, std::size_t index) {
    return std::string(path) + "[" + std::to_string(index) + "]";
}

[[nodiscard]] std::string json_type_name(const JsonValue& value) {
    if (std::holds_alternative<std::nullptr_t>(value.storage)) return "null";
    if (std::holds_alternative<bool>(value.storage)) return "布尔值";
    if (std::holds_alternative<double>(value.storage)) return "数字";
    if (std::holds_alternative<std::string>(value.storage)) return "字符串";
    if (std::holds_alternative<JsonArray>(value.storage)) return "数组";
    return "对象";
}

class MapReader final {
public:
    explicit MapReader(const CanonicalJsonReadOptions& options) : options_(options) {}

    [[nodiscard]] core::MapData read(const JsonValue& root) {
        const JsonObject& object = as_object(root, "$");
        check_fields(object, "$", {
            "$schema", "header", "roads", "lanes", "laneBoundaries", "junctions",
            "laneConnections", "operationalAreas", "stations", "restrictedAreas",
            "vehicleProfiles"});

        core::MapHeader header = read_header(required(object, "header", "$"), "$.header");
        if (header.schema_version != "1.0" && header.schema_version != "1.1") {
            throw CanonicalJsonError(
                "$.header.schemaVersion",
                "不支持的 Canonical Schema 版本：'" + header.schema_version + "'");
        }
        schema_version_ = header.schema_version;

        core::MapData map{
            .header = std::move(header),
            .roads = read_array<core::Road>(object, "roads", "$", &MapReader::read_road),
            .lanes = read_array<core::Lane>(object, "lanes", "$", &MapReader::read_lane),
            .lane_boundaries = read_array<core::LaneBoundary>(
                object, "laneBoundaries", "$", &MapReader::read_lane_boundary),
            .junctions = read_array<core::Junction>(
                object, "junctions", "$", &MapReader::read_junction),
            .lane_connections = read_array<core::LaneConnection>(
                object, "laneConnections", "$", &MapReader::read_lane_connection),
            .operational_areas = read_array<core::OperationalArea>(
                object, "operationalAreas", "$", &MapReader::read_operational_area),
            .stations = read_array<core::Station>(
                object, "stations", "$", &MapReader::read_station),
            .restricted_areas = read_array<core::RestrictedArea>(
                object, "restrictedAreas", "$", &MapReader::read_restricted_area),
            .vehicle_profiles = read_array<core::VehicleProfile>(
                object, "vehicleProfiles", "$", &MapReader::read_vehicle_profile),
        };

        if (options_.expected_map_id && map.header.map_id != *options_.expected_map_id) {
            throw CanonicalJsonError(
                "$.header.mapId",
                "地图 ID 为 '" + map.header.map_id.value() + "'，与期望的 '" +
                    options_.expected_map_id->value() + "' 不一致");
        }
        return map;
    }

private:
    using FieldNames = std::initializer_list<std::string_view>;

    [[nodiscard]] const JsonObject& as_object(
        const JsonValue& value, std::string_view path) const {
        if (const auto* object = std::get_if<JsonObject>(&value.storage)) {
            return *object;
        }
        throw CanonicalJsonError(
            std::string(path), "应为对象，实际为" + json_type_name(value));
    }

    [[nodiscard]] const JsonArray& as_array(
        const JsonValue& value, std::string_view path) const {
        if (const auto* array = std::get_if<JsonArray>(&value.storage)) {
            return *array;
        }
        throw CanonicalJsonError(
            std::string(path), "应为数组，实际为" + json_type_name(value));
    }

    [[nodiscard]] const JsonValue& required(
        const JsonObject& object,
        std::string_view name,
        std::string_view path) const {
        const auto found = std::find_if(
            object.begin(), object.end(),
            [name](const auto& member) { return member.first == name; });
        if (found == object.end()) {
            throw CanonicalJsonError(child_path(path, name), "缺少必填字段");
        }
        return found->second;
    }

    [[nodiscard]] const JsonValue* optional(
        const JsonObject& object, std::string_view name) const noexcept {
        const auto found = std::find_if(
            object.begin(), object.end(),
            [name](const auto& member) { return member.first == name; });
        return found == object.end() ? nullptr : &found->second;
    }

    void check_fields(
        const JsonObject& object,
        std::string_view path,
        FieldNames known_fields) const {
        if (!options_.reject_unknown_fields) {
            return;
        }
        for (const auto& [name, unused] : object) {
            static_cast<void>(unused);
            if (std::find(known_fields.begin(), known_fields.end(), name) == known_fields.end()) {
                throw CanonicalJsonError(child_path(path, name), "存在未知字段");
            }
        }
    }

    [[nodiscard]] std::string read_string(
        const JsonValue& value, std::string_view path) const {
        if (const auto* string = std::get_if<std::string>(&value.storage)) {
            return *string;
        }
        throw CanonicalJsonError(
            std::string(path), "应为字符串，实际为" + json_type_name(value));
    }

    [[nodiscard]] double read_number(
        const JsonValue& value, std::string_view path) const {
        if (const auto* number = std::get_if<double>(&value.storage)) {
            return *number;
        }
        throw CanonicalJsonError(
            std::string(path), "应为数字，实际为" + json_type_name(value));
    }

    [[nodiscard]] bool read_bool(
        const JsonValue& value, std::string_view path) const {
        if (const auto* boolean = std::get_if<bool>(&value.storage)) {
            return *boolean;
        }
        throw CanonicalJsonError(
            std::string(path), "应为布尔值，实际为" + json_type_name(value));
    }

    [[nodiscard]] std::uint32_t read_uint32(
        const JsonValue& value, std::string_view path) const {
        const double number = read_number(value, path);
        if (number < 0.0 || number > std::numeric_limits<std::uint32_t>::max() ||
            std::floor(number) != number) {
            throw CanonicalJsonError(std::string(path), "应为 uint32 范围内的非负整数");
        }
        return static_cast<std::uint32_t>(number);
    }

    template <typename Enum>
    [[nodiscard]] Enum read_enum(
        const JsonValue& value,
        std::string_view path,
        std::initializer_list<std::pair<std::string_view, Enum>> choices) const {
        const std::string name = read_string(value, path);
        const auto found = std::find_if(
            choices.begin(), choices.end(),
            [&name](const auto& choice) { return choice.first == name; });
        if (found == choices.end()) {
            throw CanonicalJsonError(std::string(path), "不支持的枚举值：'" + name + "'");
        }
        return found->second;
    }

    [[nodiscard]] core::Point3d read_point(
        const JsonValue& value, std::string_view path) const {
        const JsonArray& coordinates = as_array(value, path);
        if (coordinates.size() != 3U) {
            throw CanonicalJsonError(std::string(path), "三维点必须恰好包含 3 个坐标");
        }
        return {
            read_number(coordinates[0], index_path(path, 0)),
            read_number(coordinates[1], index_path(path, 1)),
            read_number(coordinates[2], index_path(path, 2)),
        };
    }

    [[nodiscard]] core::Polyline3d read_polyline(
        const JsonValue& value, std::string_view path) const {
        const JsonArray& points = as_array(value, path);
        core::Polyline3d result;
        result.reserve(points.size());
        for (std::size_t index = 0; index < points.size(); ++index) {
            result.push_back(read_point(points[index], index_path(path, index)));
        }
        return result;
    }

    [[nodiscard]] core::CurveSegment3d read_curve_segment(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        const std::string type = read_string(
            required(object, "type", path), child_path(path, "type"));
        if (type == "line") {
            check_fields(object, path, {"type", "start", "headingRad", "lengthM", "endZM"});
            return core::LineCurveSegment3d{
                .start = read_point(required(object, "start", path), child_path(path, "start")),
                .heading_rad = read_number(required(object, "headingRad", path), child_path(path, "headingRad")),
                .length_m = read_number(required(object, "lengthM", path), child_path(path, "lengthM")),
                .end_z_m = read_number(required(object, "endZM", path), child_path(path, "endZM")),
            };
        }
        if (type == "circular_arc") {
            check_fields(object, path, {"type", "start", "headingRad", "lengthM", "endZM", "curvaturePerM"});
            return core::CircularArcSegment3d{
                .start = read_point(required(object, "start", path), child_path(path, "start")),
                .heading_rad = read_number(required(object, "headingRad", path), child_path(path, "headingRad")),
                .length_m = read_number(required(object, "lengthM", path), child_path(path, "lengthM")),
                .end_z_m = read_number(required(object, "endZM", path), child_path(path, "endZM")),
                .curvature_per_m = read_number(required(object, "curvaturePerM", path), child_path(path, "curvaturePerM")),
            };
        }
        if (type == "clothoid") {
            check_fields(object, path, {
                "type", "start", "headingRad", "lengthM", "endZM",
                "startCurvaturePerM", "endCurvaturePerM"});
            return core::ClothoidSegment3d{
                .start = read_point(required(object, "start", path), child_path(path, "start")),
                .heading_rad = read_number(required(object, "headingRad", path), child_path(path, "headingRad")),
                .length_m = read_number(required(object, "lengthM", path), child_path(path, "lengthM")),
                .end_z_m = read_number(required(object, "endZM", path), child_path(path, "endZM")),
                .start_curvature_per_m = read_number(
                    required(object, "startCurvaturePerM", path), child_path(path, "startCurvaturePerM")),
                .end_curvature_per_m = read_number(
                    required(object, "endCurvaturePerM", path), child_path(path, "endCurvaturePerM")),
            };
        }
        throw CanonicalJsonError(child_path(path, "type"), "不支持的曲线段类型：'" + type + "'");
    }

    [[nodiscard]] core::PathGeometry3d read_path_geometry(
        const JsonValue& value, std::string_view path) const {
        if (std::holds_alternative<JsonArray>(value.storage)) {
            return core::PathGeometry3d{read_polyline(value, path)};
        }
        if (schema_version_ == "1.0") {
            throw CanonicalJsonError(
                std::string(path), "Schema 1.0 的路径几何只能使用点列数组");
        }
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"type", "segments"});
        const std::string type = read_string(
            required(object, "type", path), child_path(path, "type"));
        if (type != "composite_curve") {
            throw CanonicalJsonError(child_path(path, "type"), "路径对象的 type 必须为 'composite_curve'");
        }
        const std::string segments_path = child_path(path, "segments");
        const JsonArray& values = as_array(required(object, "segments", path), segments_path);
        std::vector<core::CurveSegment3d> segments;
        segments.reserve(values.size());
        for (std::size_t index = 0U; index < values.size(); ++index) {
            segments.push_back(read_curve_segment(values[index], index_path(segments_path, index)));
        }
        return core::PathGeometry3d{core::CompositeCurve3d{.segments = std::move(segments)}};
    }

    [[nodiscard]] std::vector<core::ObjectId> read_ids(
        const JsonValue& value, std::string_view path) const {
        const JsonArray& ids = as_array(value, path);
        std::vector<core::ObjectId> result;
        result.reserve(ids.size());
        for (std::size_t index = 0; index < ids.size(); ++index) {
            result.push_back(read_string(ids[index], index_path(path, index)));
        }
        return result;
    }

    template <typename Element>
    [[nodiscard]] std::vector<Element> read_array(
        const JsonObject& parent,
        std::string_view field,
        std::string_view parent_path,
        Element (MapReader::*reader)(const JsonValue&, std::string_view) const) const {
        const std::string path = child_path(parent_path, field);
        const JsonArray& array = as_array(required(parent, field, parent_path), path);
        std::vector<Element> result;
        result.reserve(array.size());
        for (std::size_t index = 0; index < array.size(); ++index) {
            result.push_back((this->*reader)(array[index], index_path(path, index)));
        }
        return result;
    }

    [[nodiscard]] core::MapHeader read_header(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"mapId", "name", "schemaVersion", "coordinateReference"});
        const std::string coordinate_path = child_path(path, "coordinateReference");
        const JsonObject& coordinate = as_object(
            required(object, "coordinateReference", path), coordinate_path);
        check_fields(
            coordinate, coordinate_path,
            {"geodeticDatum", "origin", "localFrame", "linearUnit", "angleUnit"});
        const std::string origin_path = child_path(coordinate_path, "origin");
        const JsonObject& origin = as_object(required(coordinate, "origin", coordinate_path), origin_path);
        check_fields(origin, origin_path, {"longitudeDeg", "latitudeDeg", "altitudeM"});

        return {
            .map_id = core::MapId{read_string(required(object, "mapId", path), child_path(path, "mapId"))},
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .schema_version = read_string(
                required(object, "schemaVersion", path), child_path(path, "schemaVersion")),
            .coordinate_reference = core::CoordinateReference{
                .geodetic_datum = read_string(
                    required(coordinate, "geodeticDatum", coordinate_path),
                    child_path(coordinate_path, "geodeticDatum")),
                .origin = core::GeodeticPoint{
                    .longitude_deg = read_number(
                        required(origin, "longitudeDeg", origin_path),
                        child_path(origin_path, "longitudeDeg")),
                    .latitude_deg = read_number(
                        required(origin, "latitudeDeg", origin_path),
                        child_path(origin_path, "latitudeDeg")),
                    .altitude_m = read_number(
                        required(origin, "altitudeM", origin_path),
                        child_path(origin_path, "altitudeM")),
                },
                .local_frame = read_enum<core::LocalCoordinateFrame>(
                    required(coordinate, "localFrame", coordinate_path),
                    child_path(coordinate_path, "localFrame"),
                    {{"enu", core::LocalCoordinateFrame::enu}}),
                .linear_unit = read_string(
                    required(coordinate, "linearUnit", coordinate_path),
                    child_path(coordinate_path, "linearUnit")),
                .angle_unit = read_string(
                    required(coordinate, "angleUnit", coordinate_path),
                    child_path(coordinate_path, "angleUnit")),
            },
        };
    }

    [[nodiscard]] core::Road read_road(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {
            "id", "name", "referenceLine", "predecessorIds", "successorIds", "laneIds"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .reference_line = read_path_geometry(
                required(object, "referenceLine", path), child_path(path, "referenceLine")),
            .predecessor_ids = read_ids(
                required(object, "predecessorIds", path), child_path(path, "predecessorIds")),
            .successor_ids = read_ids(
                required(object, "successorIds", path), child_path(path, "successorIds")),
            .lane_ids = read_ids(required(object, "laneIds", path), child_path(path, "laneIds")),
        };
    }

    [[nodiscard]] core::Lane read_lane(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {
            "id", "roadId", "centerline", "side", "orderFromReference", "leftBoundaryId",
            "rightBoundaryId", "predecessorIds", "successorIds", "direction", "status",
            "widthM", "speedLimitMps"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .road_id = read_string(required(object, "roadId", path), child_path(path, "roadId")),
            .centerline = read_path_geometry(
                required(object, "centerline", path), child_path(path, "centerline")),
            .side = read_enum<core::LaneSide>(
                required(object, "side", path), child_path(path, "side"),
                {{"left", core::LaneSide::left}, {"right", core::LaneSide::right}}),
            .order_from_reference = read_uint32(
                required(object, "orderFromReference", path), child_path(path, "orderFromReference")),
            .left_boundary_id = read_string(
                required(object, "leftBoundaryId", path), child_path(path, "leftBoundaryId")),
            .right_boundary_id = read_string(
                required(object, "rightBoundaryId", path), child_path(path, "rightBoundaryId")),
            .predecessor_ids = read_ids(
                required(object, "predecessorIds", path), child_path(path, "predecessorIds")),
            .successor_ids = read_ids(
                required(object, "successorIds", path), child_path(path, "successorIds")),
            .direction = read_enum<core::LaneDirection>(
                required(object, "direction", path), child_path(path, "direction"),
                {{"along_reference_line", core::LaneDirection::along_reference_line},
                 {"against_reference_line", core::LaneDirection::against_reference_line}}),
            .status = read_enum<core::LaneStatus>(
                required(object, "status", path), child_path(path, "status"),
                {{"open", core::LaneStatus::open}, {"closed", core::LaneStatus::closed}}),
            .width_m = read_number(required(object, "widthM", path), child_path(path, "widthM")),
            .speed_limit_mps = read_number(
                required(object, "speedLimitMps", path), child_path(path, "speedLimitMps")),
        };
    }

    [[nodiscard]] core::LaneBoundary read_lane_boundary(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"id", "geometry", "type", "crossingAllowed"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .geometry = read_path_geometry(
                required(object, "geometry", path), child_path(path, "geometry")),
            .type = read_enum<core::LaneBoundaryType>(
                required(object, "type", path), child_path(path, "type"),
                {{"unknown", core::LaneBoundaryType::unknown},
                 {"dashed_line", core::LaneBoundaryType::dashed_line},
                 {"solid_line", core::LaneBoundaryType::solid_line},
                 {"double_solid_line", core::LaneBoundaryType::double_solid_line},
                 {"curb", core::LaneBoundaryType::curb},
                 {"virtual_boundary", core::LaneBoundaryType::virtual_boundary}}),
            .crossing_allowed = read_bool(
                required(object, "crossingAllowed", path), child_path(path, "crossingAllowed")),
        };
    }

    [[nodiscard]] core::Junction read_junction(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"id", "name", "connectionIds"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .connection_ids = read_ids(
                required(object, "connectionIds", path), child_path(path, "connectionIds")),
        };
    }

    [[nodiscard]] core::LaneConnection read_lane_connection(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {
            "id", "junctionId", "incomingLaneId", "connectingLaneId", "outgoingLaneId",
            "turnDirection"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .junction_id = read_string(
                required(object, "junctionId", path), child_path(path, "junctionId")),
            .incoming_lane_id = read_string(
                required(object, "incomingLaneId", path), child_path(path, "incomingLaneId")),
            .connecting_lane_id = read_string(
                required(object, "connectingLaneId", path), child_path(path, "connectingLaneId")),
            .outgoing_lane_id = read_string(
                required(object, "outgoingLaneId", path), child_path(path, "outgoingLaneId")),
            .turn_direction = read_enum<core::TurnDirection>(
                required(object, "turnDirection", path), child_path(path, "turnDirection"),
                {{"straight", core::TurnDirection::straight},
                 {"left", core::TurnDirection::left},
                 {"right", core::TurnDirection::right},
                 {"u_turn", core::TurnDirection::u_turn}}),
        };
    }

    [[nodiscard]] core::OperationalArea read_operational_area(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"id", "name", "type", "outline"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .type = read_enum<core::OperationalAreaType>(
                required(object, "type", path), child_path(path, "type"),
                {{"unknown", core::OperationalAreaType::unknown},
                 {"warehouse", core::OperationalAreaType::warehouse},
                 {"loading_area", core::OperationalAreaType::loading_area},
                 {"parking_area", core::OperationalAreaType::parking_area},
                 {"charging_area", core::OperationalAreaType::charging_area}}),
            .outline = read_polyline(required(object, "outline", path), child_path(path, "outline")),
        };
    }

    [[nodiscard]] core::Station read_station(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"id", "name", "type", "position", "accessLaneId"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .type = read_enum<core::StationType>(
                required(object, "type", path), child_path(path, "type"),
                {{"unknown", core::StationType::unknown},
                 {"gate", core::StationType::gate},
                 {"loading_bay", core::StationType::loading_bay},
                 {"parking_space", core::StationType::parking_space},
                 {"charging_point", core::StationType::charging_point},
                 {"waypoint", core::StationType::waypoint}}),
            .position = read_point(required(object, "position", path), child_path(path, "position")),
            .access_lane_id = read_string(
                required(object, "accessLaneId", path), child_path(path, "accessLaneId")),
        };
    }

    [[nodiscard]] core::RestrictedArea read_restricted_area(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {"id", "name", "outline", "allowedVehicleProfileIds"});
        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .outline = read_polyline(required(object, "outline", path), child_path(path, "outline")),
            .allowed_vehicle_profile_ids = read_ids(
                required(object, "allowedVehicleProfileIds", path),
                child_path(path, "allowedVehicleProfileIds")),
        };
    }

    [[nodiscard]] core::VehicleProfile read_vehicle_profile(
        const JsonValue& value, std::string_view path) const {
        const JsonObject& object = as_object(value, path);
        check_fields(object, path, {
            "id", "name", "type", "widthM", "heightM", "lengthM", "minimumTurningRadiusM"});

        const auto read_optional_number = [&](std::string_view field) -> std::optional<double> {
            const JsonValue* value_or_null = optional(object, field);
            if (value_or_null == nullptr || std::holds_alternative<std::nullptr_t>(value_or_null->storage)) {
                return std::nullopt;
            }
            return read_number(*value_or_null, child_path(path, field));
        };

        return {
            .id = read_string(required(object, "id", path), child_path(path, "id")),
            .name = read_string(required(object, "name", path), child_path(path, "name")),
            .type = read_enum<core::VehicleType>(
                required(object, "type", path), child_path(path, "type"),
                {{"passenger_car", core::VehicleType::passenger_car},
                 {"delivery_van", core::VehicleType::delivery_van},
                 {"truck", core::VehicleType::truck}}),
            .width_m = read_number(required(object, "widthM", path), child_path(path, "widthM")),
            .height_m = read_optional_number("heightM"),
            .length_m = read_optional_number("lengthM"),
            .minimum_turning_radius_m = read_optional_number("minimumTurningRadiusM"),
        };
    }

    const CanonicalJsonReadOptions& options_;
    std::string schema_version_{"1.0"};
};

[[nodiscard]] JsonValue string_value(std::string_view value) {
    return JsonValue{std::string(value)};
}

[[nodiscard]] JsonValue number_value(double value) {
    return JsonValue{value};
}

[[nodiscard]] JsonValue bool_value(bool value) {
    return JsonValue{value};
}

[[nodiscard]] JsonValue object_value(JsonObject value) {
    return JsonValue{std::move(value)};
}

[[nodiscard]] JsonValue point_value(const core::Point3d& point) {
    return JsonValue{JsonArray{
        number_value(point.x), number_value(point.y), number_value(point.z)}};
}

[[nodiscard]] JsonValue polyline_value(const core::Polyline3d& polyline) {
    JsonArray points;
    points.reserve(polyline.size());
    for (const core::Point3d& point : polyline) {
        points.push_back(point_value(point));
    }
    return JsonValue{std::move(points)};
}

[[nodiscard]] JsonValue curve_segment_value(const core::CurveSegment3d& segment) {
    return std::visit([](const auto& value) -> JsonValue {
        using Segment = std::decay_t<decltype(value)>;
        JsonObject object{
            {"type", string_value(
                std::is_same_v<Segment, core::LineCurveSegment3d> ? "line" :
                std::is_same_v<Segment, core::CircularArcSegment3d> ? "circular_arc" : "clothoid")},
            {"start", point_value(value.start)},
            {"headingRad", number_value(value.heading_rad)},
            {"lengthM", number_value(value.length_m)},
            {"endZM", number_value(value.end_z_m)},
        };
        if constexpr (std::is_same_v<Segment, core::CircularArcSegment3d>) {
            object.emplace_back("curvaturePerM", number_value(value.curvature_per_m));
        } else if constexpr (std::is_same_v<Segment, core::ClothoidSegment3d>) {
            object.emplace_back("startCurvaturePerM", number_value(value.start_curvature_per_m));
            object.emplace_back("endCurvaturePerM", number_value(value.end_curvature_per_m));
        }
        return object_value(std::move(object));
    }, segment);
}

[[nodiscard]] JsonValue path_geometry_value(
    const core::PathGeometry3d& path,
    std::string_view json_path,
    std::string_view schema_version) {
    if (const core::Polyline3d* polyline = path.polyline()) {
        return polyline_value(*polyline);
    }
    if (schema_version == "1.0") {
        throw CanonicalJsonError(
            std::string(json_path), "曲线几何不能写入 Canonical Schema 1.0");
    }
    JsonArray segments;
    for (const core::CurveSegment3d& segment : path.composite_curve()->segments) {
        segments.push_back(curve_segment_value(segment));
    }
    return object_value({
        {"type", string_value("composite_curve")},
        {"segments", JsonValue{std::move(segments)}},
    });
}

[[nodiscard]] JsonValue ids_value(const std::vector<core::ObjectId>& ids) {
    JsonArray values;
    values.reserve(ids.size());
    for (const core::ObjectId& id : ids) {
        values.push_back(string_value(id));
    }
    return JsonValue{std::move(values)};
}

template <typename Element, typename Converter>
[[nodiscard]] JsonValue array_value(const std::vector<Element>& elements, Converter converter) {
    JsonArray values;
    values.reserve(elements.size());
    for (const Element& element : elements) {
        values.push_back(converter(element));
    }
    return JsonValue{std::move(values)};
}

[[nodiscard]] JsonValue header_value(
    const core::MapHeader& header,
    std::string_view schema_version) {
    return object_value({
        {"mapId", string_value(header.map_id.value())},
        {"name", string_value(header.name)},
        {"schemaVersion", string_value(schema_version)},
        {"coordinateReference", object_value({
            {"geodeticDatum", string_value(header.coordinate_reference.geodetic_datum)},
            {"origin", object_value({
                {"longitudeDeg", number_value(header.coordinate_reference.origin.longitude_deg)},
                {"latitudeDeg", number_value(header.coordinate_reference.origin.latitude_deg)},
                {"altitudeM", number_value(header.coordinate_reference.origin.altitude_m)},
            })},
            {"localFrame", string_value(core::local_coordinate_frame_name(
                header.coordinate_reference.local_frame))},
            {"linearUnit", string_value(header.coordinate_reference.linear_unit)},
            {"angleUnit", string_value(header.coordinate_reference.angle_unit)},
        })},
    });
}

[[nodiscard]] JsonValue road_value(
    const core::Road& road,
    std::size_t index,
    std::string_view schema_version) {
    return object_value({
        {"id", string_value(road.id)},
        {"name", string_value(road.name)},
        {"referenceLine", path_geometry_value(
            road.reference_line, "$.roads[" + std::to_string(index) + "].referenceLine", schema_version)},
        {"predecessorIds", ids_value(road.predecessor_ids)},
        {"successorIds", ids_value(road.successor_ids)},
        {"laneIds", ids_value(road.lane_ids)},
    });
}

[[nodiscard]] JsonValue lane_value(
    const core::Lane& lane,
    std::size_t index,
    std::string_view schema_version) {
    return object_value({
        {"id", string_value(lane.id)},
        {"roadId", string_value(lane.road_id)},
        {"centerline", path_geometry_value(
            lane.centerline, "$.lanes[" + std::to_string(index) + "].centerline", schema_version)},
        {"side", string_value(core::lane_side_name(lane.side))},
        {"orderFromReference", number_value(static_cast<double>(lane.order_from_reference))},
        {"leftBoundaryId", string_value(lane.left_boundary_id)},
        {"rightBoundaryId", string_value(lane.right_boundary_id)},
        {"predecessorIds", ids_value(lane.predecessor_ids)},
        {"successorIds", ids_value(lane.successor_ids)},
        {"direction", string_value(core::lane_direction_name(lane.direction))},
        {"status", string_value(core::lane_status_name(lane.status))},
        {"widthM", number_value(lane.width_m)},
        {"speedLimitMps", number_value(lane.speed_limit_mps)},
    });
}

[[nodiscard]] JsonValue lane_boundary_value(
    const core::LaneBoundary& boundary,
    std::size_t index,
    std::string_view schema_version) {
    return object_value({
        {"id", string_value(boundary.id)},
        {"geometry", path_geometry_value(
            boundary.geometry, "$.laneBoundaries[" + std::to_string(index) + "].geometry", schema_version)},
        {"type", string_value(core::lane_boundary_type_name(boundary.type))},
        {"crossingAllowed", bool_value(boundary.crossing_allowed)},
    });
}

[[nodiscard]] JsonValue junction_value(const core::Junction& junction) {
    return object_value({
        {"id", string_value(junction.id)},
        {"name", string_value(junction.name)},
        {"connectionIds", ids_value(junction.connection_ids)},
    });
}

[[nodiscard]] JsonValue lane_connection_value(const core::LaneConnection& connection) {
    return object_value({
        {"id", string_value(connection.id)},
        {"junctionId", string_value(connection.junction_id)},
        {"incomingLaneId", string_value(connection.incoming_lane_id)},
        {"connectingLaneId", string_value(connection.connecting_lane_id)},
        {"outgoingLaneId", string_value(connection.outgoing_lane_id)},
        {"turnDirection", string_value(core::turn_direction_name(connection.turn_direction))},
    });
}

[[nodiscard]] JsonValue operational_area_value(const core::OperationalArea& area) {
    return object_value({
        {"id", string_value(area.id)},
        {"name", string_value(area.name)},
        {"type", string_value(core::operational_area_type_name(area.type))},
        {"outline", polyline_value(area.outline)},
    });
}

[[nodiscard]] JsonValue station_value(const core::Station& station) {
    return object_value({
        {"id", string_value(station.id)},
        {"name", string_value(station.name)},
        {"type", string_value(core::station_type_name(station.type))},
        {"position", point_value(station.position)},
        {"accessLaneId", string_value(station.access_lane_id)},
    });
}

[[nodiscard]] JsonValue restricted_area_value(const core::RestrictedArea& area) {
    return object_value({
        {"id", string_value(area.id)},
        {"name", string_value(area.name)},
        {"outline", polyline_value(area.outline)},
        {"allowedVehicleProfileIds", ids_value(area.allowed_vehicle_profile_ids)},
    });
}

[[nodiscard]] JsonValue optional_number_value(const std::optional<double>& value) {
    return value ? number_value(*value) : JsonValue{nullptr};
}

[[nodiscard]] JsonValue vehicle_profile_value(const core::VehicleProfile& profile) {
    return object_value({
        {"id", string_value(profile.id)},
        {"name", string_value(profile.name)},
        {"type", string_value(core::vehicle_type_name(profile.type))},
        {"widthM", number_value(profile.width_m)},
        {"heightM", optional_number_value(profile.height_m)},
        {"lengthM", optional_number_value(profile.length_m)},
        {"minimumTurningRadiusM", optional_number_value(profile.minimum_turning_radius_m)},
    });
}

[[nodiscard]] JsonValue map_value(
    const core::MapData& map,
    std::string_view schema_version) {
    JsonArray roads;
    roads.reserve(map.roads.size());
    for (std::size_t index = 0U; index < map.roads.size(); ++index) {
        roads.push_back(road_value(map.roads[index], index, schema_version));
    }
    JsonArray lanes;
    lanes.reserve(map.lanes.size());
    for (std::size_t index = 0U; index < map.lanes.size(); ++index) {
        lanes.push_back(lane_value(map.lanes[index], index, schema_version));
    }
    JsonArray boundaries;
    boundaries.reserve(map.lane_boundaries.size());
    for (std::size_t index = 0U; index < map.lane_boundaries.size(); ++index) {
        boundaries.push_back(lane_boundary_value(
            map.lane_boundaries[index], index, schema_version));
    }
    return object_value({
        {"header", header_value(map.header, schema_version)},
        {"roads", JsonValue{std::move(roads)}},
        {"lanes", JsonValue{std::move(lanes)}},
        {"laneBoundaries", JsonValue{std::move(boundaries)}},
        {"junctions", array_value(map.junctions, junction_value)},
        {"laneConnections", array_value(map.lane_connections, lane_connection_value)},
        {"operationalAreas", array_value(map.operational_areas, operational_area_value)},
        {"stations", array_value(map.stations, station_value)},
        {"restrictedAreas", array_value(map.restricted_areas, restricted_area_value)},
        {"vehicleProfiles", array_value(map.vehicle_profiles, vehicle_profile_value)},
    });
}

[[nodiscard]] std::string read_file_text(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw CanonicalJsonError("$", "无法打开文件：" + path.string());
    }
    std::ostringstream content;
    content << stream.rdbuf();
    if (stream.bad()) {
        throw CanonicalJsonError("$", "读取文件失败：" + path.string());
    }
    return content.str();
}

}  // namespace

CanonicalJsonError::CanonicalJsonError(std::string json_path, std::string message)
    : std::runtime_error(json_path + "：" + message), json_path_(std::move(json_path)) {}

const std::string& CanonicalJsonError::json_path() const noexcept {
    return json_path_;
}

core::MapData read_canonical_json(
    std::string_view json,
    const CanonicalJsonReadOptions& options) {
    if (json.size() >= 3U &&
        static_cast<unsigned char>(json[0]) == 0xEFU &&
        static_cast<unsigned char>(json[1]) == 0xBBU &&
        static_cast<unsigned char>(json[2]) == 0xBFU) {
        json.remove_prefix(3U);
    }
    try {
        return MapReader{options}.read(detail::parse_json(json));
    } catch (const detail::JsonSyntaxError& error) {
        throw CanonicalJsonError("$", std::string("JSON 语法错误：") + error.what());
    }
}

core::MapData read_canonical_json_file(
    const std::filesystem::path& path,
    const CanonicalJsonReadOptions& options) {
    return read_canonical_json(read_file_text(path), options);
}

std::string write_canonical_json(
    const core::MapData& map,
    const CanonicalJsonWriteOptions& options) {
    const auto version_name = [](CanonicalJsonSchemaVersion version) -> std::string_view {
        return version == CanonicalJsonSchemaVersion::v1_0 ? "1.0" : "1.1";
    };
    const std::string schema_version = options.target_schema_version
        ? std::string(version_name(*options.target_schema_version))
        : map.header.schema_version;
    if (schema_version != "1.0" && schema_version != "1.1") {
        throw CanonicalJsonError(
            "$.header.schemaVersion",
            "不支持的 Canonical Schema 版本：'" + schema_version + "'");
    }
    try {
        return detail::serialize_json(
            map_value(map, schema_version), options.pretty_print,
            static_cast<unsigned int>(options.indent_spaces));
    } catch (const detail::JsonSyntaxError& error) {
        throw CanonicalJsonError("$", std::string("JSON 写出失败：") + error.what());
    }
}

void write_canonical_json_file(
    const std::filesystem::path& path,
    const core::MapData& map,
    const CanonicalJsonWriteOptions& options) {
    const std::string content = write_canonical_json(map, options);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw CanonicalJsonError("$", "无法创建文件：" + path.string());
    }
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!stream) {
        throw CanonicalJsonError("$", "写入文件失败：" + path.string());
    }
}

std::string_view canonical_json_format_name() noexcept {
    return "AutoMapOps Canonical JSON";
}

std::string_view canonical_json_media_type() noexcept {
    return "application/json";
}

std::string_view canonical_json_schema_version() noexcept {
    return "1.1";
}

}  // namespace automap::io
