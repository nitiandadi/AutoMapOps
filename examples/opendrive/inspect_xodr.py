"""用 Python 标准库快速检查 OpenDRIVE 文件的教学脚本。"""

from __future__ import annotations

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET


def width_at(width_element: ET.Element, ds: float) -> float:
    """计算 width 元素定义的三次多项式在局部距离 ds 处的值。"""
    a = float(width_element.attrib["a"])
    b = float(width_element.attrib["b"])
    c = float(width_element.attrib["c"])
    d = float(width_element.attrib["d"])
    return a + b * ds + c * ds**2 + d * ds**3


def inspect_xodr(path: Path) -> None:
    root = ET.parse(path).getroot()
    header = root.find("header")

    if root.tag != "OpenDRIVE" or header is None:
        raise ValueError("这不是有效的 OpenDRIVE XML 骨架")

    print(f"文件: {path}")
    print(
        "版本: "
        f"{header.attrib.get('revMajor', '?')}."
        f"{header.attrib.get('revMinor', '?')}"
    )

    roads = root.findall("road")
    print(f"道路数量: {len(roads)}")

    for road in roads:
        print(
            f"\nRoad {road.attrib['id']}: "
            f"{road.attrib.get('name', '')}, "
            f"length={road.attrib['length']} m"
        )

        speed = road.find("type/speed")
        if speed is not None:
            print(
                f"  限速: {speed.attrib['max']} "
                f"{speed.attrib.get('unit', '')}"
            )

        print("  参考线几何:")
        total_geometry_length = 0.0
        for geometry in road.findall("planView/geometry"):
            geometry_type = next(iter(geometry)).tag
            geometry_length = float(geometry.attrib["length"])
            total_geometry_length += geometry_length
            extra = ""
            if geometry_type == "arc":
                curvature = float(geometry.find("arc").attrib["curvature"])
                extra = f", curvature={curvature}, radius={1 / curvature:.3f} m"
            print(
                f"    s={geometry.attrib['s']}: {geometry_type}, "
                f"length={geometry_length} m{extra}"
            )

        road_length = float(road.attrib["length"])
        if abs(total_geometry_length - road_length) > 1e-9:
            raise ValueError(
                f"Road {road.attrib['id']} 的 geometry 总长 "
                f"{total_geometry_length} 与 road length {road_length} 不一致"
            )

        print("  车道截面:")
        for section in road.findall("lanes/laneSection"):
            lane_descriptions: list[str] = []
            for side in ("left", "center", "right"):
                for lane in section.findall(f"{side}/lane"):
                    lane_descriptions.append(
                        f"{lane.attrib['id']}({lane.attrib['type']})"
                    )
            print(
                f"    s={section.attrib['s']}: "
                + ", ".join(lane_descriptions)
            )

        lane_minus_two = road.find(
            "lanes/laneSection[@s='80.0']/right/lane[@id='-2']"
        )
        if lane_minus_two is not None:
            first_width = lane_minus_two.findall("width")[0]
            print("  lane -2 展宽检查:")
            for ds in (0.0, 10.0, 20.0, 30.0, 40.0):
                print(f"    ds={ds:>4.0f} m -> width={width_at(first_width, ds):.3f} m")


def main() -> None:
    default_map = (
        Path(__file__).resolve().parents[2]
        / "maps"
        / "opendrive"
        / "curved_multilane_road.xodr"
    )
    parser = argparse.ArgumentParser(description="检查 OpenDRIVE 教学地图")
    parser.add_argument(
        "xodr",
        nargs="?",
        type=Path,
        default=default_map,
        help="要检查的 .xodr 文件路径",
    )
    args = parser.parse_args()
    inspect_xodr(args.xodr.resolve())


if __name__ == "__main__":
    main()
