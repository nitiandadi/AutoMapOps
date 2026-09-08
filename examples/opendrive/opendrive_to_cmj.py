"""将常规 OpenDRIVE 道路网络转换为 CMJ 1.1。

转换目标是用于地图可视化、对象查询和车道拓扑学习，不承诺与 OpenDRIVE
字段级无损往返：CMJ 1.1 没有车道类型、分段宽度、高程多项式和完整道路
标线属性。Road 参考线会保留 line/arc/spiral 曲线；Lane 与 LaneBoundary
根据 OpenDRIVE 的 laneOffset、width 和 elevation 采样为三维点列。
"""

from __future__ import annotations

import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
from dataclasses import dataclass, field
import json
import math
from pathlib import Path
import re
import xml.etree.ElementTree as ET


EPSILON = 1.0e-9


def number(element: ET.Element, name: str, default: float = 0.0) -> float:
    value = element.get(name)
    return float(value) if value not in {None, ""} else default


def clean(value: float) -> float:
    if abs(value) < 1.0e-12:
        return 0.0
    return round(value, 9)


def point(x: float, y: float, z: float) -> list[float]:
    return [clean(x), clean(y), clean(z)]


def lane_token(lane_id: str) -> str:
    return f"m{lane_id[1:]}" if lane_id.startswith("-") else f"p{lane_id}"


def normalize_angle(value: float) -> float:
    return (value + math.pi) % (2.0 * math.pi) - math.pi


@dataclass(frozen=True)
class CubicRecord:
    start: float
    a: float
    b: float
    c: float
    d: float

    def value(self, position: float) -> float:
        ds = position - self.start
        return self.a + ds * (self.b + ds * (self.c + ds * self.d))


def cubic_records(
    nodes: list[ET.Element], start_attribute: str, absolute_offset: float = 0.0
) -> list[CubicRecord]:
    return sorted(
        [
            CubicRecord(
                absolute_offset + number(node, start_attribute),
                number(node, "a"),
                number(node, "b"),
                number(node, "c"),
                number(node, "d"),
            )
            for node in nodes
        ],
        key=lambda record: record.start,
    )


def cubic_value(records: list[CubicRecord], position: float, default: float = 0.0) -> float:
    if not records:
        return default
    index = bisect_right([record.start for record in records], position + EPSILON) - 1
    if index < 0:
        return default
    return records[index].value(position)


@dataclass(frozen=True)
class PlanGeometry:
    s: float
    x: float
    y: float
    heading: float
    length: float
    kind: str
    curvature: float = 0.0
    start_curvature: float = 0.0
    end_curvature: float = 0.0


@dataclass
class RoadSource:
    node: ET.Element
    odr_id: str
    canonical_id: str
    length: float
    geometries: list[PlanGeometry]
    elevations: list[CubicRecord]
    lane_offsets: list[CubicRecord]
    sections: list[ET.Element]


@dataclass
class LanePiece:
    canonical_id: str
    road_id: str
    source_lane_id: str
    section_index: int
    part_index: int
    s0: float
    s1: float
    direction: str
    centerline: list[list[float]]
    source_lane: ET.Element
    predecessors: set[str] = field(default_factory=set)
    successors: set[str] = field(default_factory=set)

    def travel_start_endpoint(self) -> str:
        return "start" if self.direction == "along_reference_line" else "end"

    def travel_end_endpoint(self) -> str:
        return "end" if self.direction == "along_reference_line" else "start"


def load_root(path: Path) -> ET.Element:
    # RoadRunner 导出的部分 XODR 在 XML 声明前带有空行，ElementTree 需先移除。
    text = path.read_text(encoding="utf-8-sig").lstrip()
    return ET.fromstring(text)


def parse_plan_view(road: ET.Element) -> list[PlanGeometry]:
    result: list[PlanGeometry] = []
    for node in road.findall("./planView/geometry"):
        child = next(iter(node), None)
        if child is None:
            raise ValueError(f"Road {road.get('id')} 的 geometry 缺少曲线类型")
        kind = child.tag
        if kind not in {"line", "arc", "spiral"}:
            raise ValueError(
                f"Road {road.get('id')} s={node.get('s')} 使用 CMJ 转换器暂不支持的 {kind}"
            )
        result.append(
            PlanGeometry(
                s=number(node, "s"),
                x=number(node, "x"),
                y=number(node, "y"),
                heading=number(node, "hdg"),
                length=number(node, "length"),
                kind=kind,
                curvature=number(child, "curvature"),
                start_curvature=number(child, "curvStart"),
                end_curvature=number(child, "curvEnd"),
            )
        )
    if not result:
        raise ValueError(f"Road {road.get('id')} 缺少 planView geometry")
    return sorted(result, key=lambda geometry: geometry.s)


def spiral_xy(geometry: PlanGeometry, ds: float) -> tuple[float, float]:
    if ds <= 0.0:
        return geometry.x, geometry.y
    # 确定性复合 Simpson 积分。当前示例没有 spiral，但通用转换器保留支持。
    steps = max(2, math.ceil(ds / 0.25))
    if steps % 2:
        steps += 1
    delta_curvature = geometry.end_curvature - geometry.start_curvature

    def heading_at(u: float) -> float:
        return (
            geometry.heading
            + geometry.start_curvature * u
            + 0.5 * delta_curvature * u * u / geometry.length
        )

    step = ds / steps
    sum_x = 0.0
    sum_y = 0.0
    for index in range(steps + 1):
        weight = 1 if index in {0, steps} else 4 if index % 2 else 2
        heading = heading_at(index * step)
        sum_x += weight * math.cos(heading)
        sum_y += weight * math.sin(heading)
    return geometry.x + step * sum_x / 3.0, geometry.y + step * sum_y / 3.0


def geometry_state(geometry: PlanGeometry, ds: float) -> tuple[float, float, float, float]:
    ds = min(max(ds, 0.0), geometry.length)
    if geometry.kind == "line":
        return (
            geometry.x + ds * math.cos(geometry.heading),
            geometry.y + ds * math.sin(geometry.heading),
            geometry.heading,
            0.0,
        )
    if geometry.kind == "arc":
        curvature = geometry.curvature
        heading = geometry.heading + curvature * ds
        if abs(curvature) < EPSILON:
            return (
                geometry.x + ds * math.cos(geometry.heading),
                geometry.y + ds * math.sin(geometry.heading),
                geometry.heading,
                0.0,
            )
        return (
            geometry.x + (math.sin(heading) - math.sin(geometry.heading)) / curvature,
            geometry.y - (math.cos(heading) - math.cos(geometry.heading)) / curvature,
            heading,
            curvature,
        )
    x, y = spiral_xy(geometry, ds)
    curvature = geometry.start_curvature + (
        geometry.end_curvature - geometry.start_curvature
    ) * ds / geometry.length
    heading = geometry.heading + geometry.start_curvature * ds + 0.5 * (
        geometry.end_curvature - geometry.start_curvature
    ) * ds * ds / geometry.length
    return x, y, heading, curvature


def road_state(road: RoadSource, s: float) -> tuple[float, float, float, float, float]:
    s = min(max(s, 0.0), road.length)
    starts = [geometry.s for geometry in road.geometries]
    index = max(0, bisect_right(starts, s + EPSILON) - 1)
    geometry = road.geometries[index]
    x, y, heading, curvature = geometry_state(geometry, s - geometry.s)
    z = cubic_value(road.elevations, s)
    return x, y, z, heading, curvature


def reference_curve(road: RoadSource, maximum_z_span: float = 10.0) -> dict:
    segments: list[dict] = []
    elevation_starts = [record.start for record in road.elevations]
    for geometry in road.geometries:
        end = min(road.length, geometry.s + geometry.length)
        cuts = {geometry.s, end}
        cuts.update(start for start in elevation_starts if geometry.s < start < end)
        cursor = geometry.s
        while cursor + maximum_z_span < end - EPSILON:
            cursor += maximum_z_span
            cuts.add(cursor)
        ordered = sorted(cuts)
        for start, finish in zip(ordered, ordered[1:]):
            length = finish - start
            if length <= EPSILON:
                continue
            x, y, z, heading, _ = road_state(road, start)
            end_z = road_state(road, finish)[2]
            local_start = start - geometry.s
            if geometry.kind == "line":
                segment = {
                    "type": "line",
                    "start": point(x, y, z),
                    "headingRad": clean(heading),
                    "lengthM": clean(length),
                    "endZM": clean(end_z),
                }
            elif geometry.kind == "arc" and abs(geometry.curvature) >= EPSILON:
                segment = {
                    "type": "circular_arc",
                    "start": point(x, y, z),
                    "headingRad": clean(heading),
                    "lengthM": clean(length),
                    "endZM": clean(end_z),
                    "curvaturePerM": clean(geometry.curvature),
                }
            else:
                curvature_delta = geometry.end_curvature - geometry.start_curvature
                start_curvature = geometry.start_curvature + curvature_delta * local_start / geometry.length
                end_curvature = geometry.start_curvature + curvature_delta * (local_start + length) / geometry.length
                segment = {
                    "type": "clothoid",
                    "start": point(x, y, z),
                    "headingRad": clean(heading),
                    "lengthM": clean(length),
                    "endZM": clean(end_z),
                    "startCurvaturePerM": clean(start_curvature),
                    "endCurvaturePerM": clean(end_curvature),
                }
            segments.append(segment)
    return {"type": "composite_curve", "segments": segments}


def lane_width_records(lane: ET.Element, section_start: float) -> list[CubicRecord]:
    return cubic_records(lane.findall("width"), "sOffset", section_start)


def lane_width(lane: ET.Element, section_start: float, s: float) -> float:
    return max(0.0, cubic_value(lane_width_records(lane, section_start), s))


def side_lanes(section: ET.Element, side: str) -> list[ET.Element]:
    lanes = section.findall(f"./{side}/lane")
    return sorted(lanes, key=lambda lane: abs(int(lane.get("id", "0"))))


def lateral_ranges(
    road: RoadSource, section: ET.Element, section_start: float, side: str, s: float
) -> dict[str, tuple[float, float, float]]:
    base = cubic_value(road.lane_offsets, s)
    accumulated = 0.0
    ranges: dict[str, tuple[float, float, float]] = {}
    for lane in side_lanes(section, side):
        width = lane_width(lane, section_start, s)
        inner = base + accumulated if side == "left" else base - accumulated
        outer = inner + width if side == "left" else inner - width
        ranges[lane.get("id", "")] = (inner, outer, width)
        accumulated += width
    return ranges


def world_point(road: RoadSource, s: float, lateral_offset: float) -> list[float]:
    x, y, z, heading, _ = road_state(road, s)
    return point(
        x - math.sin(heading) * lateral_offset,
        y + math.cos(heading) * lateral_offset,
        z,
    )


def sample_positions(
    road: RoadSource,
    section: ET.Element,
    section_start: float,
    start: float,
    end: float,
    maximum_step: float,
) -> list[float]:
    mandatory = {start, end}
    for value in (
        [geometry.s for geometry in road.geometries]
        + [geometry.s + geometry.length for geometry in road.geometries]
        + [record.start for record in road.elevations]
        + [record.start for record in road.lane_offsets]
    ):
        if start < value < end:
            mandatory.add(value)
    for lane in side_lanes(section, "left") + side_lanes(section, "right"):
        for record in lane_width_records(lane, section_start):
            if start < record.start < end:
                mandatory.add(record.start)
    result: list[float] = []
    ordered = sorted(mandatory)
    for interval_start, interval_end in zip(ordered, ordered[1:]):
        if not result:
            result.append(interval_start)
        span = interval_end - interval_start
        steps = max(1, math.ceil(span / maximum_step))
        result.extend(interval_start + span * index / steps for index in range(1, steps + 1))
    return result


def compact_polyline(points: list[list[float]]) -> list[list[float]]:
    compact: list[list[float]] = []
    for value in points:
        if not compact or any(abs(a - b) > 1.0e-9 for a, b in zip(value, compact[-1])):
            compact.append(value)
    if len(compact) == 1:
        compact.append(compact[0].copy())
    return compact


def active_road_mark(lane: ET.Element | None, local_s: float) -> ET.Element | None:
    if lane is None:
        return None
    marks = sorted(lane.findall("roadMark"), key=lambda mark: number(mark, "sOffset"))
    active = None
    for mark in marks:
        if number(mark, "sOffset") <= local_s + EPSILON:
            active = mark
        else:
            break
    return active


def boundary_style(mark: ET.Element | None) -> tuple[str, bool]:
    if mark is None:
        return "virtual_boundary", True
    mark_type = mark.get("type", "none")
    lane_change = mark.get("laneChange", "none")
    crossing_allowed = lane_change in {"both", "increase", "decrease"}
    if mark_type == "none":
        return "virtual_boundary", True
    if mark_type == "curb":
        return "curb", False
    if mark_type == "broken":
        return "dashed_line", crossing_allowed
    if mark_type in {"solid solid", "solid broken", "broken solid", "broken broken"}:
        return "double_solid_line", crossing_allowed and "broken" in mark_type
    if mark_type == "solid":
        return "solid_line", False
    return "unknown", crossing_allowed


def lane_direction(lane: ET.Element) -> str:
    vector_lane = lane.find("./userData/vectorLane")
    if vector_lane is not None:
        travel_direction = vector_lane.get("travelDir")
        if travel_direction == "forward":
            return "along_reference_line"
        if travel_direction == "backward":
            return "against_reference_line"
    return "along_reference_line" if int(lane.get("id", "0")) < 0 else "against_reference_line"


def speed_mps(road: ET.Element, lane: ET.Element, local_s: float, road_s: float) -> float:
    speed_node = None
    for candidate in sorted(lane.findall("speed"), key=lambda node: number(node, "sOffset")):
        if number(candidate, "sOffset") <= local_s + EPSILON:
            speed_node = candidate
    unit = speed_node.get("unit", "m/s") if speed_node is not None else None
    if speed_node is None:
        road_type = None
        for candidate in sorted(road.findall("type"), key=lambda node: number(node, "s")):
            if number(candidate, "s") <= road_s + EPSILON:
                road_type = candidate
        speed_node = road_type.find("speed") if road_type is not None else None
        unit = speed_node.get("unit", "m/s") if speed_node is not None else "m/s"
    if speed_node is None:
        return 0.0
    maximum = speed_node.get("max", "0")
    try:
        value = float(maximum)
    except ValueError:
        return 0.0
    conversions = {"m/s": 1.0, "mps": 1.0, "km/h": 1.0 / 3.6, "kmh": 1.0 / 3.6, "mph": 0.44704}
    return value * conversions.get(unit or "m/s", 1.0)


def geodetic_origin(header: ET.Element | None) -> tuple[float, float, float]:
    text = header.findtext("geoReference", "") if header is not None else ""

    def parameter(name: str) -> float:
        match = re.search(rf"(?:^|\s)\+{name}=([^\s]+)", text)
        return float(match.group(1)) if match else 0.0

    return parameter("lon_0"), parameter("lat_0"), 0.0


def road_endpoint(road: RoadSource, junction_id: str) -> str | None:
    predecessor = road.node.find("./link/predecessor")
    if predecessor is not None and predecessor.get("elementType") == "junction" and predecessor.get("elementId") == junction_id:
        return "start"
    successor = road.node.find("./link/successor")
    if successor is not None and successor.get("elementType") == "junction" and successor.get("elementId") == junction_id:
        return "end"
    return None


def turn_direction(incoming: LanePiece, connecting: LanePiece) -> str:
    def heading(piece: LanePiece, endpoint: str, leaving: bool) -> float:
        points = piece.centerline
        if endpoint == "start":
            dx = points[1][0] - points[0][0]
            dy = points[1][1] - points[0][1]
            if not leaving:
                dx, dy = -dx, -dy
        else:
            dx = points[-1][0] - points[-2][0]
            dy = points[-1][1] - points[-2][1]
            if leaving:
                dx, dy = -dx, -dy
        return math.atan2(dy, dx)

    incoming_heading = heading(incoming, incoming.travel_end_endpoint(), False)
    connecting_heading = heading(connecting, connecting.travel_start_endpoint(), True)
    delta = normalize_angle(connecting_heading - incoming_heading)
    if abs(delta) < math.radians(35.0):
        return "straight"
    if abs(delta) > math.radians(145.0):
        return "u_turn"
    return "left" if delta > 0.0 else "right"


def convert(root: ET.Element, map_id: str, name: str, sample_step: float) -> tuple[dict, dict]:
    source_roads = root.findall("road")
    road_ids = {node.get("id", ""): f"road_odr_{node.get('id', '')}" for node in source_roads}
    roads_by_id: dict[str, RoadSource] = {}
    for node in source_roads:
        odr_id = node.get("id", "")
        roads_by_id[odr_id] = RoadSource(
            node=node,
            odr_id=odr_id,
            canonical_id=road_ids[odr_id],
            length=number(node, "length"),
            geometries=parse_plan_view(node),
            elevations=cubic_records(node.findall("./elevationProfile/elevation"), "s"),
            lane_offsets=cubic_records(node.findall("./lanes/laneOffset"), "s"),
            sections=sorted(node.findall("./lanes/laneSection"), key=lambda section: number(section, "s")),
        )

    cmj_roads: list[dict] = []
    cmj_lanes: list[dict] = []
    cmj_boundaries: list[dict] = []
    road_values: dict[str, dict] = {}
    pieces: dict[str, LanePiece] = {}
    pieces_by_source: dict[tuple[str, int, str], list[LanePiece]] = defaultdict(list)
    boundary_cache: dict[tuple[str, int, int, str, int], str] = {}
    excluded_lane_types: Counter[str] = Counter()

    for road in roads_by_id.values():
        road_value = {
            "id": road.canonical_id,
            "name": road.node.get("name", "") or road.canonical_id,
            "referenceLine": reference_curve(road),
            "predecessorIds": [],
            "successorIds": [],
            "laneIds": [],
        }
        cmj_roads.append(road_value)
        road_values[road.odr_id] = road_value

        for section_index, section in enumerate(road.sections):
            section_start = number(section, "s")
            section_end = number(road.sections[section_index + 1], "s") if section_index + 1 < len(road.sections) else road.length
            cuts = {section_start, section_end}
            all_lanes = side_lanes(section, "left") + side_lanes(section, "right")
            for source_lane in all_lanes:
                source_lane_type = source_lane.get("type", "none")
                if source_lane_type != "driving":
                    excluded_lane_types[source_lane_type] += 1
            center_lane = section.find("./center/lane[@id='0']")
            for lane in all_lanes + ([center_lane] if center_lane is not None else []):
                for mark in lane.findall("roadMark"):
                    absolute = section_start + number(mark, "sOffset")
                    if section_start < absolute < section_end:
                        cuts.add(absolute)
            ordered_cuts = sorted(cuts)

            for part_index, (part_start, part_end) in enumerate(zip(ordered_cuts, ordered_cuts[1:])):
                if part_end - part_start <= EPSILON:
                    continue
                positions = sample_positions(road, section, section_start, part_start, part_end, sample_step)
                midpoint = 0.5 * (part_start + part_end)
                for side in ("left", "right"):
                    ordered_side_lanes = side_lanes(section, side)
                    for lane_order, lane in enumerate(ordered_side_lanes, start=1):
                        lane_type = lane.get("type", "none")
                        if lane_type != "driving":
                            continue
                        source_lane_id = lane.get("id", "")
                        direction = lane_direction(lane)
                        lane_id = (
                            f"lane_odr_{road.odr_id}_s{section_index}_p{part_index}_"
                            f"{lane_token(source_lane_id)}"
                        )
                        center_points: list[list[float]] = []
                        for s in positions:
                            inner, outer, _ = lateral_ranges(road, section, section_start, side, s)[source_lane_id]
                            center_points.append(world_point(road, s, 0.5 * (inner + outer)))
                        centerline = compact_polyline(center_points)

                        def ensure_boundary(boundary_order: int) -> str:
                            key_side = "center" if boundary_order == 0 else side
                            key = (road.odr_id, section_index, part_index, key_side, boundary_order)
                            if key in boundary_cache:
                                return boundary_cache[key]
                            boundary_id = (
                                f"boundary_odr_{road.odr_id}_s{section_index}_p{part_index}_"
                                f"{key_side}_b{boundary_order}"
                            )
                            boundary_points: list[list[float]] = []
                            for s in positions:
                                if boundary_order == 0:
                                    offset = cubic_value(road.lane_offsets, s)
                                else:
                                    provider = ordered_side_lanes[boundary_order - 1]
                                    offset = lateral_ranges(road, section, section_start, side, s)[provider.get("id", "")][1]
                                boundary_points.append(world_point(road, s, offset))
                            provider_lane = center_lane if boundary_order == 0 else ordered_side_lanes[boundary_order - 1]
                            mark = active_road_mark(provider_lane, midpoint - section_start)
                            style, crossing = boundary_style(mark)
                            cmj_boundaries.append(
                                {
                                    "id": boundary_id,
                                    "geometry": compact_polyline(boundary_points),
                                    "type": style,
                                    "crossingAllowed": crossing,
                                }
                            )
                            boundary_cache[key] = boundary_id
                            return boundary_id

                        inner_id = ensure_boundary(lane_order - 1)
                        outer_id = ensure_boundary(lane_order)
                        if direction == "along_reference_line":
                            left_id, right_id = (
                                (outer_id, inner_id) if side == "left" else (inner_id, outer_id)
                            )
                        else:
                            left_id, right_id = (
                                (inner_id, outer_id) if side == "left" else (outer_id, inner_id)
                            )
                        width = lane_width(lane, section_start, midpoint)
                        piece = LanePiece(
                            canonical_id=lane_id,
                            road_id=road.odr_id,
                            source_lane_id=source_lane_id,
                            section_index=section_index,
                            part_index=part_index,
                            s0=part_start,
                            s1=part_end,
                            direction=direction,
                            centerline=centerline,
                            source_lane=lane,
                        )
                        pieces[lane_id] = piece
                        pieces_by_source[(road.odr_id, section_index, source_lane_id)].append(piece)
                        road_value["laneIds"].append(lane_id)
                        cmj_lanes.append(
                            {
                                "id": lane_id,
                                "roadId": road.canonical_id,
                                "centerline": centerline,
                                "side": side,
                                "orderFromReference": lane_order,
                                "leftBoundaryId": left_id,
                                "rightBoundaryId": right_id,
                                "predecessorIds": [],
                                "successorIds": [],
                                "direction": direction,
                                "status": "open",
                                "widthM": clean(width),
                                "speedLimitMps": clean(speed_mps(road.node, lane, midpoint - section_start, midpoint)),
                            }
                        )

    def ordered_pieces(road_id: str, section_index: int, source_lane_id: str) -> list[LanePiece]:
        return sorted(
            pieces_by_source.get((road_id, section_index, source_lane_id), []),
            key=lambda piece: (piece.s0, piece.s1),
        )

    topology_warnings: list[str] = []

    def connect_physical(first: LanePiece, first_endpoint: str, second: LanePiece, second_endpoint: str) -> None:
        if first_endpoint == first.travel_end_endpoint() and second_endpoint == second.travel_start_endpoint():
            first.successors.add(second.canonical_id)
            second.predecessors.add(first.canonical_id)
        elif second_endpoint == second.travel_end_endpoint() and first_endpoint == first.travel_start_endpoint():
            second.successors.add(first.canonical_id)
            first.predecessors.add(second.canonical_id)
        else:
            topology_warnings.append(
                f"行驶方向不匹配：{first.canonical_id}:{first_endpoint} ↔ {second.canonical_id}:{second_endpoint}"
            )

    # 同一 laneSection 内因标线变化切出的连续 CMJ Lane。
    for source_key, source_pieces in pieces_by_source.items():
        ordered = sorted(source_pieces, key=lambda piece: (piece.s0, piece.s1))
        for first, second in zip(ordered, ordered[1:]):
            connect_physical(first, "end", second, "start")

    # 相邻 laneSection 的 lane link。
    for road in roads_by_id.values():
        for section_index in range(len(road.sections) - 1):
            current = road.sections[section_index]
            following = road.sections[section_index + 1]
            following_ids = {
                lane.get("id", "")
                for lane in side_lanes(following, "left") + side_lanes(following, "right")
                if lane.get("type") == "driving"
            }
            for lane in side_lanes(current, "left") + side_lanes(current, "right"):
                if lane.get("type") != "driving":
                    continue
                successor = lane.find("./link/successor")
                target_id = successor.get("id", "") if successor is not None else lane.get("id", "")
                if target_id not in following_ids:
                    continue
                source_values = ordered_pieces(road.odr_id, section_index, lane.get("id", ""))
                target_values = ordered_pieces(road.odr_id, section_index + 1, target_id)
                if source_values and target_values:
                    connect_physical(source_values[-1], "end", target_values[0], "start")

        # 一些 RoadRunner 文件含有约 1e-10 m 的退化 laneSection。它们不会生成
        # CMJ Lane，但相邻的同 ID Lane 仍应跨过这些空截面保持连续。
        by_lane_id: dict[str, list[LanePiece]] = defaultdict(list)
        for (piece_road_id, _, source_lane_id), source_values in pieces_by_source.items():
            if piece_road_id == road.odr_id and source_values:
                by_lane_id[source_lane_id].extend(source_values)
        for source_values in by_lane_id.values():
            section_groups: dict[int, list[LanePiece]] = defaultdict(list)
            for piece in source_values:
                section_groups[piece.section_index].append(piece)
            populated_sections = sorted(section_groups)
            for first_index, second_index in zip(populated_sections, populated_sections[1:]):
                if second_index <= first_index + 1:
                    continue
                first = max(section_groups[first_index], key=lambda piece: piece.s1)
                second = min(section_groups[second_index], key=lambda piece: piece.s0)
                if 0.0 <= second.s0 - first.s1 <= 1.0e-6:
                    connect_physical(first, "end", second, "start")

    def endpoint_piece(road_id: str, lane_id: str, endpoint: str) -> LanePiece | None:
        road = roads_by_id.get(road_id)
        if road is None:
            return None
        section_indices = range(len(road.sections)) if endpoint == "start" else range(len(road.sections) - 1, -1, -1)
        for section_index in section_indices:
            values = ordered_pieces(road_id, section_index, lane_id)
            if values:
                return values[0] if endpoint == "start" else values[-1]
        return None

    # 普通 Road link 形成的跨道路车道连接及 Road 拓扑。
    for road in roads_by_id.values():
        for relation, endpoint in (("predecessor", "start"), ("successor", "end")):
            link = road.node.find(f"./link/{relation}")
            if link is None or link.get("elementType") != "road":
                continue
            target_road_id = link.get("elementId", "")
            target_endpoint = link.get("contactPoint", "start")
            if relation == "predecessor":
                road_values[road.odr_id]["predecessorIds"].append(road_ids[target_road_id])
            else:
                road_values[road.odr_id]["successorIds"].append(road_ids[target_road_id])
            section_index = 0 if endpoint == "start" else len(road.sections) - 1
            section = road.sections[section_index]
            for lane in side_lanes(section, "left") + side_lanes(section, "right"):
                if lane.get("type") != "driving":
                    continue
                lane_link = lane.find(f"./link/{relation}")
                if lane_link is None:
                    continue
                first = endpoint_piece(road.odr_id, lane.get("id", ""), endpoint)
                second = endpoint_piece(target_road_id, lane_link.get("id", ""), target_endpoint)
                if first is not None and second is not None:
                    connect_physical(first, endpoint, second, target_endpoint)

    cmj_junctions: list[dict] = []
    cmj_connections: list[dict] = []
    unresolved_connections: list[str] = []
    for junction in root.findall("junction"):
        junction_odr_id = junction.get("id", "")
        junction_id = f"junction_odr_{junction_odr_id}"
        connection_ids: list[str] = []
        for connection in junction.findall("connection"):
            incoming_road_id = connection.get("incomingRoad", "")
            connecting_road_id = connection.get("connectingRoad", "")
            contact_point = connection.get("contactPoint", "start")
            incoming_road = roads_by_id.get(incoming_road_id)
            connecting_road = roads_by_id.get(connecting_road_id)
            if incoming_road is None or connecting_road is None:
                unresolved_connections.append(f"Junction {junction_odr_id} 引用了不存在的 Road")
                continue
            incoming_endpoint = road_endpoint(incoming_road, junction_odr_id)
            if incoming_endpoint is None:
                unresolved_connections.append(
                    f"Junction {junction_odr_id} 无法判断 incomingRoad {incoming_road_id} 的连接端"
                )
                continue

            # Junction 对 Road 参考方向的物理连接。
            if incoming_endpoint == "end":
                road_values[incoming_road_id]["successorIds"].append(road_ids[connecting_road_id])
            else:
                road_values[incoming_road_id]["predecessorIds"].append(road_ids[connecting_road_id])
            if contact_point == "start":
                road_values[connecting_road_id]["predecessorIds"].append(road_ids[incoming_road_id])
            else:
                road_values[connecting_road_id]["successorIds"].append(road_ids[incoming_road_id])

            opposite = "end" if contact_point == "start" else "start"
            opposite_relation = "successor" if opposite == "end" else "predecessor"
            outgoing_link = connecting_road.node.find(f"./link/{opposite_relation}")
            outgoing_road_id = (
                outgoing_link.get("elementId", "")
                if outgoing_link is not None and outgoing_link.get("elementType") == "road"
                else ""
            )
            outgoing_endpoint = outgoing_link.get("contactPoint", "start") if outgoing_link is not None else "start"

            for link_index, lane_link in enumerate(connection.findall("laneLink")):
                incoming_lane_id = lane_link.get("from", "")
                connecting_lane_id = lane_link.get("to", "")
                incoming_piece = endpoint_piece(incoming_road_id, incoming_lane_id, incoming_endpoint)
                connecting_piece = endpoint_piece(connecting_road_id, connecting_lane_id, contact_point)
                if incoming_piece is None or connecting_piece is None:
                    unresolved_connections.append(
                        f"Junction {junction_odr_id} connection {connection.get('id')} 缺少可行驶 Lane 映射"
                    )
                    continue
                connect_physical(incoming_piece, incoming_endpoint, connecting_piece, contact_point)

                connector_endpoint_piece = endpoint_piece(connecting_road_id, connecting_lane_id, opposite)
                connector_lane_node = connector_endpoint_piece.source_lane if connector_endpoint_piece is not None else None
                outgoing_lane_link = connector_lane_node.find(f"./link/{opposite_relation}") if connector_lane_node is not None else None
                outgoing_piece = (
                    endpoint_piece(outgoing_road_id, outgoing_lane_link.get("id", ""), outgoing_endpoint)
                    if outgoing_lane_link is not None and outgoing_road_id
                    else None
                )
                if outgoing_piece is None:
                    unresolved_connections.append(
                        f"Junction {junction_odr_id} connection {connection.get('id')} 无法解析 outgoingLane"
                    )
                    continue
                connection_id = f"connection_j{junction_odr_id}_{connection.get('id', '0')}_{link_index}"
                connection_ids.append(connection_id)
                cmj_connections.append(
                    {
                        "id": connection_id,
                        "junctionId": junction_id,
                        "incomingLaneId": incoming_piece.canonical_id,
                        "connectingLaneId": connecting_piece.canonical_id,
                        "outgoingLaneId": outgoing_piece.canonical_id,
                        "turnDirection": turn_direction(incoming_piece, connecting_piece),
                    }
                )
        cmj_junctions.append(
            {
                "id": junction_id,
                "name": junction.get("name", "") or junction_id,
                "connectionIds": connection_ids,
            }
        )

    lane_values = {lane["id"]: lane for lane in cmj_lanes}
    for piece in pieces.values():
        lane_values[piece.canonical_id]["predecessorIds"] = sorted(piece.predecessors)
        lane_values[piece.canonical_id]["successorIds"] = sorted(piece.successors)
    for road_value in cmj_roads:
        road_value["predecessorIds"] = sorted(set(road_value["predecessorIds"]))
        road_value["successorIds"] = sorted(set(road_value["successorIds"]))

    longitude, latitude, altitude = geodetic_origin(root.find("header"))
    result = {
        "$schema": "../../schemas/canonical-map-1.1.schema.json",
        "header": {
            "mapId": map_id,
            "name": name,
            "schemaVersion": "1.1",
            "coordinateReference": {
                "geodeticDatum": "WGS84",
                "origin": {
                    "longitudeDeg": clean(longitude),
                    "latitudeDeg": clean(latitude),
                    "altitudeM": clean(altitude),
                },
                "localFrame": "enu",
                "linearUnit": "m",
                "angleUnit": "rad",
            },
        },
        "roads": cmj_roads,
        "lanes": cmj_lanes,
        "laneBoundaries": cmj_boundaries,
        "junctions": cmj_junctions,
        "laneConnections": cmj_connections,
        "operationalAreas": [],
        "stations": [],
        "restrictedAreas": [],
        "vehicleProfiles": [],
    }
    report = {
        "sourceFormat": "OpenDRIVE",
        "targetFormat": "CMJ 1.1",
        "sourceCounts": {
            "roads": len(source_roads),
            "junctions": len(root.findall("junction")),
            "laneSections": sum(len(road.sections) for road in roads_by_id.values()),
        },
        "targetCounts": {
            "roads": len(cmj_roads),
            "lanes": len(cmj_lanes),
            "laneBoundaries": len(cmj_boundaries),
            "junctions": len(cmj_junctions),
            "laneConnections": len(cmj_connections),
        },
        "excludedNonDrivingLaneRecords": dict(sorted(excluded_lane_types.items())),
        "unresolvedJunctionConnections": unresolved_connections,
        "topologyDirectionWarnings": sorted(set(topology_warnings)),
        "knownLosses": [
            "CMJ 1.1 未保存 OpenDRIVE 非 driving 车道对象；其宽度仍参与可行驶车道横向定位。",
            "CMJ 1.1 的 widthM 为区段中点宽度；中心线和边界几何使用完整宽度多项式采样。",
            "道路标线颜色、宽度、材质和非对称组合未完整保留。",
            "Road 参考线 Z 使用至多 10 m 的分段线性近似；Lane 和 LaneBoundary 的采样点使用原高程多项式。",
            "OpenDRIVE 用户自定义 userData 未写入 CMJ 1.1。",
        ],
    }
    return result, report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xodr", type=Path, help="输入 OpenDRIVE .xodr 文件")
    parser.add_argument("output", type=Path, help="输出 CMJ .json 文件")
    parser.add_argument("--map-id", default=None, help="CMJ mapId；默认使用输入文件名")
    parser.add_argument("--name", default=None, help="CMJ 地图名称")
    parser.add_argument("--sample-step-m", type=float, default=1.5, help="车道几何最大采样间距，默认 1.5 m")
    parser.add_argument("--report", type=Path, default=None, help="可选的转换报告 JSON")
    args = parser.parse_args()
    if args.sample_step_m <= 0.0:
        parser.error("--sample-step-m 必须大于 0")

    root = load_root(args.xodr)
    map_id = args.map_id or args.xodr.stem
    name = args.name or root.find("header").get("name", "") or args.xodr.stem
    result, report = convert(root, map_id, name, args.sample_step_m)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if args.report is not None:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    counts = report["targetCounts"]
    print(
        f"已生成 {args.output}：{counts['roads']} 条 Road，{counts['lanes']} 条 Lane，"
        f"{counts['laneBoundaries']} 条 LaneBoundary，{counts['laneConnections']} 条 Junction 连接"
    )
    if report["unresolvedJunctionConnections"]:
        print(f"注意：{len(report['unresolvedJunctionConnections'])} 条 Junction 映射未解析，详见转换报告。")


if __name__ == "__main__":
    main()
