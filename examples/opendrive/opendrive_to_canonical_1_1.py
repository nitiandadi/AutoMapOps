"""将本项目物流园 OpenDRIVE 1.8 示例翻译为 Canonical JSON 1.1。

道路、车道、边界和 Junction 拓扑来自 XODR。OpenDRIVE userData 指向的业务
Polygon、Station 与车辆白名单从配套 Canonical V0 源文件恢复，避免把 s/t
包围盒误当成精确业务轮廓。
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET


def user_data(element: ET.Element, code: str) -> str | None:
    node = element.find(f"./userData[@code='{code}']")
    return node.get("value") if node is not None else None


def curve_segments(plan_view: ET.Element, lateral_offset: float = 0.0) -> list[dict]:
    segments: list[dict] = []
    for geometry in plan_view.findall("geometry"):
        x = float(geometry.get("x", "0"))
        y = float(geometry.get("y", "0"))
        heading = float(geometry.get("hdg", "0"))
        length = float(geometry.get("length", "0"))
        start = [
            x - math.sin(heading) * lateral_offset,
            y + math.cos(heading) * lateral_offset,
            0.0,
        ]
        common = {
            "start": start,
            "headingRad": heading,
            "endZM": 0.0,
        }
        if geometry.find("line") is not None:
            segments.append({"type": "line", **common, "lengthM": length})
            continue
        arc = geometry.find("arc")
        if arc is not None:
            curvature = float(arc.get("curvature", "0"))
            offset_scale = 1.0 - curvature * lateral_offset
            if offset_scale <= 0:
                raise ValueError(
                    f"偏移曲线半径无效：s={geometry.get('s')}，t={lateral_offset}"
                )
            segments.append(
                {
                    "type": "circular_arc",
                    **common,
                    "lengthM": length * offset_scale,
                    "curvaturePerM": curvature / offset_scale,
                }
            )
            continue
        spiral = geometry.find("spiral")
        if spiral is not None:
            start_curvature = float(spiral.get("curvStart", "0"))
            end_curvature = float(spiral.get("curvEnd", "0"))
            if lateral_offset != 0:
                raise ValueError("当前示例不支持对 spiral 车道中心线做精确横向偏移")
            segments.append(
                {
                    "type": "clothoid",
                    **common,
                    "lengthM": length,
                    "startCurvaturePerM": start_curvature,
                    "endCurvaturePerM": end_curvature,
                }
            )
            continue
        raise ValueError(f"暂不支持的 planView geometry：s={geometry.get('s')}")
    if not segments:
        raise ValueError("Road 缺少 planView geometry")
    return segments


def road_mark_type(mark: ET.Element | None) -> tuple[str, bool]:
    if mark is None:
        return "virtual_boundary", True
    mark_type = mark.get("type", "none")
    if mark_type in {"broken", "broken broken", "broken solid", "solid broken"}:
        return "dashed_line", mark.get("laneChange", "none") != "none"
    if mark_type in {"solid solid"}:
        return "double_solid_line", False
    if mark_type == "none":
        return "virtual_boundary", True
    return "solid_line", False


def canonical_road_ids(roads: list[ET.Element]) -> dict[str, str]:
    ids: dict[str, str] = {}
    used: set[str] = set()
    for road in roads:
        odr_id = road.get("id", "")
        candidate = user_data(road, "automap.canonicalRoadId") or f"road_odr_{odr_id}"
        role = user_data(road, "automap.exportRole")
        if role == "synthesized_merge_connector":
            candidate = (
                "road_j20_main_connector"
                if odr_id == "201"
                else "road_j20_detour_connector"
            )
        if candidate in used:
            candidate = f"{candidate}_odr_{odr_id}"
        used.add(candidate)
        ids[odr_id] = candidate
    return ids


def convert(xodr_path: Path, semantics_path: Path) -> dict:
    root = ET.parse(xodr_path).getroot()
    source = json.loads(semantics_path.read_text(encoding="utf-8"))
    source_road_names = {road["id"]: road["name"] for road in source["roads"]}
    source_junction_names = {
        junction["id"]: junction["name"] for junction in source["junctions"]
    }
    road_nodes = root.findall("road")
    road_ids = canonical_road_ids(road_nodes)

    lane_ids: dict[tuple[str, str], str] = {}
    used_lane_ids: set[str] = set()
    road_driving_lanes: dict[str, list[ET.Element]] = {}
    for road in road_nodes:
        odr_road_id = road.get("id", "")
        lanes = [
            lane
            for lane in road.findall("./lanes/laneSection/right/lane")
            if lane.get("type") == "driving"
        ]
        lanes.sort(key=lambda lane: abs(int(lane.get("id", "0"))))
        road_driving_lanes[odr_road_id] = lanes
        for lane in lanes:
            odr_lane_id = lane.get("id", "")
            candidate = user_data(lane, "automap.canonicalLaneId")
            role = user_data(lane, "automap.exportRole")
            if role == "synthesized_merge_connector":
                suffix = {("-1", "201"): "main_inner", ("-2", "201"): "main_outer"}.get(
                    (odr_lane_id, odr_road_id),
                    "detour",
                )
                candidate = f"lane_j20_{suffix}"
            if not candidate:
                candidate = f"lane_odr_{odr_road_id}_{odr_lane_id.replace('-', 'm')}"
            if candidate in used_lane_ids:
                candidate = f"{candidate}_odr_{odr_road_id}"
            used_lane_ids.add(candidate)
            lane_ids[(odr_road_id, odr_lane_id)] = candidate

    predecessor_ids: dict[str, set[str]] = {road_id: set() for road_id in road_ids}
    successor_ids: dict[str, set[str]] = {road_id: set() for road_id in road_ids}
    for road in road_nodes:
        odr_id = road.get("id", "")
        predecessor = road.find("./link/predecessor")
        successor = road.find("./link/successor")
        if predecessor is not None and predecessor.get("elementType") == "road":
            predecessor_ids[odr_id].add(road_ids[predecessor.get("elementId", "")])
        if successor is not None and successor.get("elementType") == "road":
            successor_ids[odr_id].add(road_ids[successor.get("elementId", "")])

    for junction in root.findall("junction"):
        for connection in junction.findall("connection"):
            incoming = connection.get("incomingRoad", "")
            connecting = connection.get("connectingRoad", "")
            successor_ids[incoming].add(road_ids[connecting])
            predecessor_ids[connecting].add(road_ids[incoming])

    roads: list[dict] = []
    lanes: list[dict] = []
    boundaries: list[dict] = []
    lane_predecessors: dict[str, set[str]] = {value: set() for value in lane_ids.values()}
    lane_successors: dict[str, set[str]] = {value: set() for value in lane_ids.values()}

    for road in road_nodes:
        odr_id = road.get("id", "")
        canonical_id = road_ids[odr_id]
        driving_lanes = road_driving_lanes[odr_id]
        boundary_ids = [f"boundary_{canonical_id.removeprefix('road_')}_{index}" for index in range(len(driving_lanes) + 1)]
        roads.append(
            {
                "id": canonical_id,
                "name": source_road_names.get(
                    canonical_id,
                    {
                        "road_j20_main_connector": "J20 主路直行连接道",
                        "road_j20_detour_connector": "J20 绕行汇流连接道",
                    }.get(canonical_id, road.get("name", canonical_id)),
                ),
                "referenceLine": {
                    "type": "composite_curve",
                    "segments": curve_segments(road.find("planView")),
                },
                "predecessorIds": sorted(predecessor_ids[odr_id]),
                "successorIds": sorted(successor_ids[odr_id]),
                "laneIds": [lane_ids[(odr_id, lane.get("id", ""))] for lane in driving_lanes],
            }
        )

        lane_zero_mark = road.find("./lanes/laneSection/center/lane[@id='0']/roadMark")
        boundary_type, crossing = road_mark_type(lane_zero_mark)
        boundaries.append(
            {
                "id": boundary_ids[0],
                "geometry": {
                    "type": "composite_curve",
                    "segments": curve_segments(road.find("planView")),
                },
                "type": boundary_type,
                "crossingAllowed": crossing,
            }
        )

        accumulated_width = 0.0
        for index, lane in enumerate(driving_lanes):
            lane_id = lane_ids[(odr_id, lane.get("id", ""))]
            width_node = lane.find("width")
            width = float(width_node.get("a", "0")) if width_node is not None else 0.0
            center_offset = -(accumulated_width + width * 0.5)
            speed_node = lane.find("speed")
            road_speed = road.find("./type/speed")
            speed = float(
                (speed_node if speed_node is not None else road_speed).get("max", "0")
            )
            lanes.append(
                {
                    "id": lane_id,
                    "roadId": canonical_id,
                    "centerline": {
                        "type": "composite_curve",
                        "segments": curve_segments(road.find("planView"), center_offset),
                    },
                    "side": "right",
                    "orderFromReference": index + 1,
                    "leftBoundaryId": boundary_ids[index],
                    "rightBoundaryId": boundary_ids[index + 1],
                    "predecessorIds": [],
                    "successorIds": [],
                    "direction": "along_reference_line",
                    "status": "open",
                    "widthM": width,
                    "speedLimitMps": speed,
                }
            )
            accumulated_width += width
            boundary_type, crossing = road_mark_type(lane.find("roadMark"))
            boundaries.append(
                {
                    "id": boundary_ids[index + 1],
                    "geometry": {
                        "type": "composite_curve",
                        "segments": curve_segments(road.find("planView"), -accumulated_width),
                    },
                    "type": boundary_type,
                    "crossingAllowed": crossing,
                }
            )

    road_by_odr = {road.get("id", ""): road for road in road_nodes}
    for road in road_nodes:
        odr_id = road.get("id", "")
        for lane in road_driving_lanes[odr_id]:
            current = lane_ids[(odr_id, lane.get("id", ""))]
            pred_lane = lane.find("./link/predecessor")
            pred_road = road.find("./link/predecessor")
            if pred_lane is not None and pred_road is not None and pred_road.get("elementType") == "road":
                key = (pred_road.get("elementId", ""), pred_lane.get("id", ""))
                if key in lane_ids:
                    lane_predecessors[current].add(lane_ids[key])
            succ_lane = lane.find("./link/successor")
            succ_road = road.find("./link/successor")
            if succ_lane is not None and succ_road is not None and succ_road.get("elementType") == "road":
                key = (succ_road.get("elementId", ""), succ_lane.get("id", ""))
                if key in lane_ids:
                    lane_successors[current].add(lane_ids[key])

    junctions: list[dict] = []
    lane_connections: list[dict] = []
    for junction_node in root.findall("junction"):
        junction_odr_id = junction_node.get("id", "")
        junction_id = user_data(junction_node, "automap.canonicalJunctionId") or f"junction_odr_{junction_odr_id}"
        connection_ids: list[str] = []
        for connection in junction_node.findall("connection"):
            incoming_road = connection.get("incomingRoad", "")
            connecting_road = connection.get("connectingRoad", "")
            connector = road_by_odr[connecting_road]
            outgoing_link = connector.find("./link/successor")
            outgoing_road = outgoing_link.get("elementId", "") if outgoing_link is not None else ""
            for link_index, lane_link in enumerate(connection.findall("laneLink")):
                incoming_key = (incoming_road, lane_link.get("from", ""))
                connecting_key = (connecting_road, lane_link.get("to", ""))
                if incoming_key not in lane_ids or connecting_key not in lane_ids:
                    continue
                incoming_lane = lane_ids[incoming_key]
                connecting_lane = lane_ids[connecting_key]
                lane_successors[incoming_lane].add(connecting_lane)
                lane_predecessors[connecting_lane].add(incoming_lane)
                connector_lane = next(
                    lane
                    for lane in road_driving_lanes[connecting_road]
                    if lane.get("id") == lane_link.get("to")
                )
                outgoing_lane_link = connector_lane.find("./link/successor")
                outgoing_key = (
                    outgoing_road,
                    outgoing_lane_link.get("id", "") if outgoing_lane_link is not None else "",
                )
                outgoing_lane = lane_ids.get(outgoing_key, connecting_lane)
                connection_id = f"connection_j{junction_odr_id}_{connection.get('id', '0')}_{link_index}"
                connection_ids.append(connection_id)
                lane_connections.append(
                    {
                        "id": connection_id,
                        "junctionId": junction_id,
                        "incomingLaneId": incoming_lane,
                        "connectingLaneId": connecting_lane,
                        "outgoingLaneId": outgoing_lane,
                        "turnDirection": "straight" if connecting_road in {"101", "201"} else "left",
                    }
                )
        junctions.append(
            {
                "id": junction_id,
                "name": source_junction_names.get(
                    junction_id,
                    "主路与装卸绕行路汇流口"
                    if junction_odr_id == "20"
                    else junction_node.get("name", junction_id),
                ),
                "connectionIds": connection_ids,
            }
        )

    lane_by_id = {lane["id"]: lane for lane in lanes}
    for lane_id, lane in lane_by_id.items():
        lane["predecessorIds"] = sorted(lane_predecessors[lane_id])
        lane["successorIds"] = sorted(lane_successors[lane_id])

    header = source["header"]
    return {
        "$schema": "../../schemas/canonical-map-1.1.schema.json",
        "header": {
            **header,
            "mapId": "logistics_park_from_opendrive",
            "name": "物流园 OpenDRIVE 1.8 转换示例",
            "schemaVersion": "1.1",
        },
        "roads": roads,
        "lanes": lanes,
        "laneBoundaries": boundaries,
        "junctions": junctions,
        "laneConnections": lane_connections,
        "operationalAreas": source["operationalAreas"],
        "stations": source["stations"],
        "restrictedAreas": source["restrictedAreas"],
        "vehicleProfiles": source["vehicleProfiles"],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xodr", type=Path)
    parser.add_argument("semantics", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = convert(args.xodr, args.semantics)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(
        f"已生成 {args.output}：{len(result['roads'])} 条 Road，"
        f"{len(result['lanes'])} 条 Lane，{len(result['laneConnections'])} 条连接"
    )


if __name__ == "__main__":
    main()
