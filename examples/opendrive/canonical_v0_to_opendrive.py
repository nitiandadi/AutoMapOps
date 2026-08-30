"""将物流园 Canonical V0 草稿导出为教学用 OpenDRIVE 1.8。

该转换器针对 maps/drafts/logistics_park_v0.json 的固定场景拓扑。它会把
Canonical 中折叠的末端汇流重新展开为 OpenDRIVE Junction 20 及两条连接道路。
只使用 Python 标准库。
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET


PI = math.pi


def number(value: float) -> str:
    return f"{value:.15g}"


def add_user_data(parent: ET.Element, code: str, value: str) -> None:
    ET.SubElement(parent, "userData", {"code": code, "value": value})


def add_line(plan_view: ET.Element, s: float, x: float, y: float,
             hdg: float, length: float) -> None:
    geometry = ET.SubElement(
        plan_view,
        "geometry",
        {
            "s": number(s),
            "x": number(x),
            "y": number(y),
            "hdg": number(hdg),
            "length": number(length),
        },
    )
    ET.SubElement(geometry, "line")


def add_arc(plan_view: ET.Element, s: float, x: float, y: float,
            hdg: float, length: float, curvature: float) -> None:
    geometry = ET.SubElement(
        plan_view,
        "geometry",
        {
            "s": number(s),
            "x": number(x),
            "y": number(y),
            "hdg": number(hdg),
            "length": number(length),
        },
    )
    ET.SubElement(geometry, "arc", {"curvature": number(curvature)})


def add_link(road: ET.Element, predecessor: dict[str, str] | None,
             successor: dict[str, str] | None) -> None:
    link = ET.SubElement(road, "link")
    if predecessor:
        ET.SubElement(link, "predecessor", predecessor)
    if successor:
        ET.SubElement(link, "successor", successor)


def add_lane_section(
    road: ET.Element,
    canonical_lane_ids: list[str],
    lane_map: dict[str, dict],
    predecessor: bool,
    successor: bool,
    junction_connector: bool = False,
    export_role: str = "canonical_lane",
) -> None:
    lanes = ET.SubElement(road, "lanes")
    section = ET.SubElement(lanes, "laneSection", {"s": "0"})
    center = ET.SubElement(section, "center")
    center_lane = ET.SubElement(
        center, "lane", {"id": "0", "type": "none", "level": "false"}
    )
    if not junction_connector:
        ET.SubElement(
            center_lane,
            "roadMark",
            {
                "sOffset": "0",
                "type": "solid",
                "weight": "standard",
                "color": "yellow",
                "width": "0.15",
                "laneChange": "none",
            },
        )

    right = ET.SubElement(section, "right")
    lane_count = len(canonical_lane_ids)
    for order, canonical_id in enumerate(canonical_lane_ids, start=1):
        source_lane = lane_map[canonical_id]
        lane = ET.SubElement(
            right,
            "lane",
            {"id": str(-order), "type": "driving", "level": "false"},
        )
        if predecessor or successor:
            lane_link = ET.SubElement(lane, "link")
            if predecessor:
                ET.SubElement(lane_link, "predecessor", {"id": str(-order)})
            if successor:
                ET.SubElement(lane_link, "successor", {"id": str(-order)})

        ET.SubElement(
            lane,
            "width",
            {
                "sOffset": "0",
                "a": number(source_lane["widthM"]),
                "b": "0",
                "c": "0",
                "d": "0",
            },
        )
        ET.SubElement(
            lane,
            "roadMark",
            {
                "sOffset": "0",
                "type": "broken" if lane_count > 1 and order == 1 else "solid",
                "weight": "standard",
                "color": "white",
                "width": "0.15",
                "laneChange": "both" if lane_count > 1 and order == 1 else "none",
            },
        )
        ET.SubElement(
            lane,
            "speed",
            {"sOffset": "0", "max": number(source_lane["speedLimitMps"])},
        )
        add_user_data(lane, "automap.canonicalLaneId", canonical_id)
        add_user_data(lane, "automap.exportRole", export_role)


def add_object(objects: ET.Element, attributes: dict[str, str],
               user_data: dict[str, str],
               parking: dict[str, str] | None = None) -> None:
    obj = ET.SubElement(objects, "object", attributes)
    if parking:
        ET.SubElement(obj, "parkingSpace", parking)
    for code, value in user_data.items():
        add_user_data(obj, code, value)


def create_road(
    root: ET.Element,
    *,
    road_id: str,
    name: str,
    length: float,
    junction: str,
    predecessor: dict[str, str] | None,
    successor: dict[str, str] | None,
    speed_mps: float,
    geometries: list[tuple],
    canonical_lane_ids: list[str],
    lane_map: dict[str, dict],
    lane_predecessor: bool,
    lane_successor: bool,
    canonical_road_id: str,
    junction_connector: bool = False,
    export_role: str = "canonical_road",
    object_specs: list[dict] | None = None,
) -> None:
    road = ET.SubElement(
        root,
        "road",
        {
            "name": name,
            "length": number(length),
            "id": road_id,
            "junction": junction,
            "rule": "RHT",
        },
    )
    add_link(road, predecessor, successor)
    road_type = ET.SubElement(road, "type", {"s": "0", "type": "town"})
    ET.SubElement(road_type, "speed", {"max": number(speed_mps), "unit": "m/s"})
    plan_view = ET.SubElement(road, "planView")
    for geometry in geometries:
        if geometry[0] == "line":
            add_line(plan_view, *geometry[1:])
        else:
            add_arc(plan_view, *geometry[1:])

    add_lane_section(
        road,
        canonical_lane_ids,
        lane_map,
        lane_predecessor,
        lane_successor,
        junction_connector,
        export_role,
    )
    if object_specs:
        objects = ET.SubElement(road, "objects")
        for spec in object_specs:
            add_object(objects, spec["attributes"], spec["userData"], spec.get("parking"))
    add_user_data(road, "automap.canonicalRoadId", canonical_road_id)
    add_user_data(road, "automap.exportRole", export_role)


def base_object_attributes(
    *, object_id: str, name: str, object_type: str, subtype: str,
    s: float, t: float, length: float, width: float,
    height: float = 0.0, orientation: str = "none",
) -> dict[str, str]:
    return {
        "id": object_id,
        "name": name,
        "type": object_type,
        "subtype": subtype,
        "s": number(s),
        "t": number(t),
        "zOffset": "0",
        "dynamic": "no",
        "orientation": orientation,
        "length": number(length),
        "width": number(width),
        "height": number(height),
        "hdg": "0",
        "pitch": "0",
        "roll": "0",
    }


def build_opendrive(data: dict) -> ET.ElementTree:
    lane_map = {lane["id"]: lane for lane in data["lanes"]}
    station_map = {station["id"]: station for station in data["stations"]}
    area_map = {area["id"]: area for area in data["operationalAreas"]}
    restricted_map = {area["id"]: area for area in data["restrictedAreas"]}

    root = ET.Element("OpenDRIVE")
    header = ET.SubElement(
        root,
        "header",
        {
            "revMajor": "1",
            "revMinor": "8",
            "name": data["header"]["name"],
            "version": "0.1.0",
            "date": "2026-08-28",
            "north": "115.0",
            "south": "-185.0",
            "east": "295.0",
            "west": "-55.0",
            "vendor": "AutoMapOps",
        },
    )
    geo_reference = ET.SubElement(header, "geoReference")
    geo_reference.text = (
        "+proj=tmerc +lat_0=30.5728 +lon_0=104.0668 +k=1 "
        "+x_0=0 +y_0=0 +ellps=WGS84 +units=m +no_defs"
    )
    add_user_data(header, "automap.sourceMapId", data["header"]["mapId"])
    add_user_data(header, "automap.sourceSchemaVersion", data["header"]["schemaVersion"])

    entry_objects = [
        {
            "attributes": base_object_attributes(
                object_id="obj_gate", name=station_map["station_gate"]["name"],
                object_type="building", subtype="tollBooth", s=10.0, t=-1.75,
                length=3.0, width=2.0, height=3.0,
            ),
            "userData": {
                "automap.canonicalStationId": "station_gate",
                "automap.stationType": "gate",
                "automap.accessLaneId": station_map["station_gate"]["accessLaneId"],
            },
        }
    ]

    detour_objects = [
        {
            "attributes": base_object_attributes(
                object_id="obj_warehouse_a", name=area_map["area_warehouse_a"]["name"],
                object_type="building", subtype="building", s=87.1238898038469,
                t=17.0, length=40.0, width=26.0, height=8.0,
            ),
            "userData": {
                "automap.canonicalOperationalAreaId": "area_warehouse_a",
                "automap.operationalAreaType": "warehouse",
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_loading_area_a", name=area_map["area_loading_a"]["name"],
                object_type="parkingSpace", subtype="openSpace", s=87.1238898038469,
                t=0.0, length=16.0, width=12.0, orientation="+",
            ),
            "parking": {"access": "all", "restrictions": "logistics_loading_area"},
            "userData": {
                "automap.canonicalOperationalAreaId": "area_loading_a",
                "automap.operationalAreaType": "loading_area",
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_loading_bay_a1", name=station_map["station_loading_a1"]["name"],
                object_type="parkingSpace", subtype="openSpace", s=87.1238898038469,
                t=-1.75, length=12.0, width=3.5, orientation="+",
            ),
            "parking": {"access": "all", "restrictions": "logistics_loading_bay"},
            "userData": {
                "automap.canonicalStationId": "station_loading_a1",
                "automap.stationType": "loading_bay",
                "automap.accessLaneId": station_map["station_loading_a1"]["accessLaneId"],
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_narrow_passage",
                name=restricted_map["restricted_narrow_passage"]["name"],
                object_type="roadSurface", subtype="other", s=15.0, t=-1.75,
                length=22.0, width=6.5,
            ),
            "userData": {
                "automap.canonicalRestrictedAreaId": "restricted_narrow_passage",
                "automap.allowedVehicleProfileIds": ",".join(
                    restricted_map["restricted_narrow_passage"]["allowedVehicleProfileIds"]
                ),
            },
        },
    ]

    return_objects = [
        {
            "attributes": base_object_attributes(
                object_id="obj_parking_area", name=area_map["area_parking"]["name"],
                object_type="parkingSpace", subtype="openSpace", s=294.57963267949,
                t=-19.0, length=45.0, width=22.0, orientation="+",
            ),
            "parking": {"access": "all"},
            "userData": {
                "automap.canonicalOperationalAreaId": "area_parking",
                "automap.operationalAreaType": "parking_area",
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_parking_01", name=station_map["station_parking_01"]["name"],
                object_type="parkingSpace", subtype="openSpace", s=295.07963267949,
                t=-16.0, length=6.0, width=3.0, orientation="+",
            ),
            "parking": {"access": "all"},
            "userData": {
                "automap.canonicalStationId": "station_parking_01",
                "automap.stationType": "parking_space",
                "automap.accessLaneId": station_map["station_parking_01"]["accessLaneId"],
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_charging_area", name=area_map["area_charging"]["name"],
                object_type="parkingSpace", subtype="openSpace", s=443.07963267949,
                t=-18.0, length=28.0, width=20.0, orientation="+",
            ),
            "parking": {"access": "electric", "restrictions": "charging_area"},
            "userData": {
                "automap.canonicalOperationalAreaId": "area_charging",
                "automap.operationalAreaType": "charging_area",
            },
        },
        {
            "attributes": base_object_attributes(
                object_id="obj_charger_01", name=station_map["station_charger_01"]["name"],
                object_type="obstacle", subtype="chargingStation", s=443.07963267949,
                t=-18.0, length=1.0, width=1.0, height=2.0,
            ),
            "userData": {
                "automap.canonicalStationId": "station_charger_01",
                "automap.stationType": "charging_point",
                "automap.accessLaneId": station_map["station_charger_01"]["accessLaneId"],
            },
        },
    ]

    roads = [
        dict(road_id="1", name="Gate Entry Road", length=50.0, junction="-1",
             predecessor={"elementType": "road", "elementId": "5", "contactPoint": "end"},
             successor={"elementType": "junction", "elementId": "10"}, speed_mps=8.33,
             geometries=[("line", 0.0, 0.0, 0.0, 0.0, 50.0)],
             canonical_lane_ids=["lane_entry_inner", "lane_entry_outer"],
             lane_predecessor=True, lane_successor=False, canonical_road_id="road_entry",
             object_specs=entry_objects),
        dict(road_id="2", name="Main Distribution Road", length=110.0, junction="-1",
             predecessor={"elementType": "junction", "elementId": "10"},
             successor={"elementType": "junction", "elementId": "20"}, speed_mps=8.33,
             geometries=[("line", 0.0, 55.0, 0.0, 0.0, 110.0)],
             canonical_lane_ids=["lane_main_inner", "lane_main_outer"],
             lane_predecessor=True, lane_successor=True, canonical_road_id="road_main"),
        dict(road_id="3", name="Loading Area Detour", length=174.247779607694, junction="-1",
             predecessor={"elementType": "junction", "elementId": "10"},
             successor={"elementType": "junction", "elementId": "20"}, speed_mps=5.56,
             geometries=[
                 ("line", 0.0, 70.0, 20.0, PI / 2, 30.0),
                 ("arc", 30.0, 70.0, 50.0, PI / 2, 15 * PI, -1 / 30),
                 ("line", 30.0 + 15 * PI, 100.0, 80.0, 0.0, 20.0),
                 ("arc", 50.0 + 15 * PI, 120.0, 80.0, 0.0, 15 * PI, -1 / 30),
                 ("line", 50.0 + 30 * PI, 150.0, 50.0, -PI / 2, 30.0),
             ], canonical_lane_ids=["lane_detour"], lane_predecessor=True,
             lane_successor=True, canonical_road_id="road_detour", object_specs=detour_objects),
        dict(road_id="4", name="Park Exit Road", length=70.0, junction="-1",
             predecessor={"elementType": "junction", "elementId": "20"},
             successor={"elementType": "road", "elementId": "5", "contactPoint": "start"},
             speed_mps=8.33, geometries=[("line", 0.0, 170.0, 0.0, 0.0, 70.0)],
             canonical_lane_ids=["lane_exit_inner", "lane_exit_outer"],
             lane_predecessor=False, lane_successor=True, canonical_road_id="road_exit"),
        dict(road_id="5", name="Return Loop Road", length=714.159265358979, junction="-1",
             predecessor={"elementType": "road", "elementId": "4", "contactPoint": "end"},
             successor={"elementType": "road", "elementId": "1", "contactPoint": "start"},
             speed_mps=8.33, geometries=[
                 ("arc", 0.0, 240.0, 0.0, 0.0, 25 * PI, -0.02),
                 ("line", 25 * PI, 290.0, -50.0, -PI / 2, 80.0),
                 ("arc", 80.0 + 25 * PI, 290.0, -130.0, -PI / 2, 25 * PI, -0.02),
                 ("line", 80.0 + 50 * PI, 240.0, -180.0, -PI, 240.0),
                 ("arc", 320.0 + 50 * PI, 0.0, -180.0, -PI, 25 * PI, -0.02),
                 ("line", 320.0 + 75 * PI, -50.0, -130.0, -3 * PI / 2, 80.0),
                 ("arc", 400.0 + 75 * PI, -50.0, -50.0, -3 * PI / 2, 25 * PI, -0.02),
             ], canonical_lane_ids=["lane_return_inner", "lane_return_outer"],
             lane_predecessor=True, lane_successor=True, canonical_road_id="road_return",
             object_specs=return_objects),
        dict(road_id="101", name="J10 Main Connector", length=5.0, junction="10",
             predecessor={"elementType": "road", "elementId": "1", "contactPoint": "end"},
             successor={"elementType": "road", "elementId": "2", "contactPoint": "start"},
             speed_mps=5.56, geometries=[("line", 0.0, 50.0, 0.0, 0.0, 5.0)],
             canonical_lane_ids=["lane_j10_main_inner", "lane_j10_main_outer"],
             lane_predecessor=True, lane_successor=True,
             canonical_road_id="road_j10_main_connector", junction_connector=True),
        dict(road_id="102", name="J10 Detour Connector", length=10 * PI, junction="10",
             predecessor={"elementType": "road", "elementId": "1", "contactPoint": "end"},
             successor={"elementType": "road", "elementId": "3", "contactPoint": "start"},
             speed_mps=5.56, geometries=[("arc", 0.0, 50.0, 0.0, 0.0, 10 * PI, 0.05)],
             canonical_lane_ids=["lane_j10_detour"], lane_predecessor=True,
             lane_successor=True, canonical_road_id="road_j10_detour_connector",
             junction_connector=True),
        dict(road_id="201", name="J20 Main Connector", length=5.0, junction="20",
             predecessor={"elementType": "road", "elementId": "2", "contactPoint": "end"},
             successor={"elementType": "road", "elementId": "4", "contactPoint": "start"},
             speed_mps=5.56, geometries=[("line", 0.0, 165.0, 0.0, 0.0, 5.0)],
             canonical_lane_ids=["lane_main_inner", "lane_main_outer"],
             lane_predecessor=True, lane_successor=True, canonical_road_id="road_main",
             junction_connector=True, export_role="synthesized_merge_connector"),
        dict(road_id="202", name="J20 Detour Merge Connector", length=10 * PI, junction="20",
             predecessor={"elementType": "road", "elementId": "3", "contactPoint": "end"},
             successor={"elementType": "road", "elementId": "4", "contactPoint": "start"},
             speed_mps=5.56,
             geometries=[("arc", 0.0, 150.0, 20.0, -PI / 2, 10 * PI, 0.05)],
             canonical_lane_ids=["lane_detour"], lane_predecessor=True,
             lane_successor=True, canonical_road_id="road_detour", junction_connector=True,
             export_role="synthesized_merge_connector"),
    ]
    for spec in roads:
        create_road(root, lane_map=lane_map, **spec)

    junction_10 = ET.SubElement(
        root, "junction", {"name": "Depot Route Split", "id": "10", "type": "default"}
    )
    connection = ET.SubElement(
        junction_10, "connection",
        {"id": "0", "incomingRoad": "1", "connectingRoad": "101", "contactPoint": "start"},
    )
    ET.SubElement(connection, "laneLink", {"from": "-1", "to": "-1"})
    ET.SubElement(connection, "laneLink", {"from": "-2", "to": "-2"})
    connection = ET.SubElement(
        junction_10, "connection",
        {"id": "1", "incomingRoad": "1", "connectingRoad": "102", "contactPoint": "start"},
    )
    ET.SubElement(connection, "laneLink", {"from": "-1", "to": "-1"})
    add_user_data(junction_10, "automap.canonicalJunctionId", "junction_route_split")

    junction_20 = ET.SubElement(
        root, "junction", {"name": "Depot Route Merge", "id": "20", "type": "default"}
    )
    connection = ET.SubElement(
        junction_20, "connection",
        {"id": "0", "incomingRoad": "2", "connectingRoad": "201", "contactPoint": "start"},
    )
    ET.SubElement(connection, "laneLink", {"from": "-1", "to": "-1"})
    ET.SubElement(connection, "laneLink", {"from": "-2", "to": "-2"})
    connection = ET.SubElement(
        junction_20, "connection",
        {"id": "1", "incomingRoad": "3", "connectingRoad": "202", "contactPoint": "start"},
    )
    ET.SubElement(connection, "laneLink", {"from": "-1", "to": "-1"})
    add_user_data(junction_20, "automap.exportRole", "synthesized_merge_junction")

    ET.indent(root, space="    ")
    return ET.ElementTree(root)


def main() -> int:
    parser = argparse.ArgumentParser(description="导出物流园 Canonical V0 为 OpenDRIVE 1.8")
    parser.add_argument("input", type=Path, help="Canonical V0 JSON 路径")
    parser.add_argument("output", type=Path, help="输出 .xodr 路径")
    args = parser.parse_args()

    with args.input.open("r", encoding="utf-8") as source:
        data = json.load(source)
    tree = build_opendrive(data)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    tree.write(args.output, encoding="utf-8", xml_declaration=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
