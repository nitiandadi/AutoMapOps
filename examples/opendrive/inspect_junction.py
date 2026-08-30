"""检查 OpenDRIVE Junction 的 connection 与 laneLink 关系。"""

from __future__ import annotations

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


def fail(message: str) -> None:
    """用面向学习者的中文信息终止检查。"""
    raise ValueError(message)


def lane_ids_at(road: ET.Element, contact_point: str) -> set[str]:
    """取得 Road 指定端点处的非零车道编号。"""
    sections = road.findall("lanes/laneSection")
    if not sections:
        fail(f"Road {road.attrib['id']} 没有 laneSection")

    section = sections[0] if contact_point == "start" else sections[-1]
    return {
        lane.attrib["id"]
        for side in ("left", "right")
        for lane in section.findall(f"{side}/lane")
    }


def linked_outgoing_road(connector: ET.Element) -> tuple[str, str]:
    """取得 Connecting Road 的后继 Road 和后继接触端点。"""
    successor = connector.find("link/successor")
    if successor is None or successor.attrib.get("elementType") != "road":
        fail(f"Connecting Road {connector.attrib['id']} 没有有效的道路后继")
    return successor.attrib["elementId"], successor.attrib["contactPoint"]


def connector_successor_lane(connector: ET.Element, connector_lane_id: str) -> str:
    """取得 Connecting Road 车道通往出口 Road 的车道编号。"""
    sections = connector.findall("lanes/laneSection")
    if not sections:
        fail(f"Connecting Road {connector.attrib['id']} 没有 laneSection")

    lane = sections[-1].find(f"./left/lane[@id='{connector_lane_id}']")
    if lane is None:
        lane = sections[-1].find(f"./right/lane[@id='{connector_lane_id}']")
    if lane is None:
        fail(
            f"Connecting Road {connector.attrib['id']} 不存在车道 {connector_lane_id}"
        )

    successor = lane.find("link/successor")
    if successor is None:
        fail(
            f"Connecting Road {connector.attrib['id']} lane {connector_lane_id} "
            "没有车道级 successor"
        )
    return successor.attrib["id"]


def inspect_junction(path: Path) -> None:
    root = ET.parse(path).getroot()
    if root.tag != "OpenDRIVE":
        fail("根节点不是 OpenDRIVE")

    roads = {road.attrib["id"]: road for road in root.findall("road")}
    junctions = root.findall("junction")
    if not junctions:
        fail("文件中没有 junction")

    print(f"文件：{path}")
    print(f"Road 数量：{len(roads)}")
    print(f"Junction 数量：{len(junctions)}")

    for junction in junctions:
        junction_id = junction.attrib["id"]
        connections = junction.findall("connection")
        print(f"\nJunction {junction_id}：{len(connections)} 条 connection")

        for connection in connections:
            connection_id = connection.attrib["id"]
            incoming_id = connection.attrib["incomingRoad"]
            connector_id = connection.attrib["connectingRoad"]
            connector_contact = connection.attrib["contactPoint"]

            if incoming_id not in roads:
                fail(f"connection {connection_id} 引用了不存在的入口 Road {incoming_id}")
            if connector_id not in roads:
                fail(
                    f"connection {connection_id} 引用了不存在的 Connecting Road "
                    f"{connector_id}"
                )

            incoming = roads[incoming_id]
            connector = roads[connector_id]
            if connector.attrib.get("junction") != junction_id:
                fail(
                    f"Connecting Road {connector_id} 的 junction 属性不是 {junction_id}"
                )

            # 本例的入口道路通过终点进入 Junction，因此读取其最后一个 laneSection。
            incoming_lane_ids = lane_ids_at(incoming, "end")
            connector_lane_ids = lane_ids_at(connector, connector_contact)
            outgoing_id, outgoing_contact = linked_outgoing_road(connector)
            if outgoing_id not in roads:
                fail(f"Connecting Road {connector_id} 的出口 Road {outgoing_id} 不存在")
            outgoing_lane_ids = lane_ids_at(roads[outgoing_id], outgoing_contact)

            lane_links = connection.findall("laneLink")
            if not lane_links:
                fail(f"connection {connection_id} 没有 laneLink")

            for lane_link in lane_links:
                from_lane = lane_link.attrib["from"]
                to_lane = lane_link.attrib["to"]
                if from_lane not in incoming_lane_ids:
                    fail(
                        f"connection {connection_id} 的 from={from_lane} "
                        f"不属于入口 Road {incoming_id}"
                    )
                if to_lane not in connector_lane_ids:
                    fail(
                        f"connection {connection_id} 的 to={to_lane} "
                        f"不属于 Connecting Road {connector_id} 的 {connector_contact} 端"
                    )

                outgoing_lane = connector_successor_lane(connector, to_lane)
                if outgoing_lane not in outgoing_lane_ids:
                    fail(
                        f"Connecting Road {connector_id} lane {to_lane} 的后继车道 "
                        f"{outgoing_lane} 不属于出口 Road {outgoing_id}"
                    )

                print(
                    f"  connection {connection_id}："
                    f"Road {incoming_id} lane {from_lane} -> "
                    f"Road {connector_id}({connector_contact}) lane {to_lane} -> "
                    f"Road {outgoing_id}({outgoing_contact}) lane {outgoing_lane}"
                )

    print("\n检查通过：所有 connection 和 laneLink 引用均可解析。")


def main() -> None:
    default_map = (
        Path(__file__).resolve().parents[2]
        / "maps"
        / "opendrive"
        / "junction_straight_left.xodr"
    )
    parser = argparse.ArgumentParser(description="检查 OpenDRIVE 路口拓扑")
    parser.add_argument(
        "xodr",
        nargs="?",
        type=Path,
        default=default_map,
        help="要检查的 .xodr 文件路径",
    )
    args = parser.parse_args()
    inspect_junction(args.xodr.resolve())


if __name__ == "__main__":
    main()
