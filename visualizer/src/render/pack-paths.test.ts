import { describe, expect, it } from "vitest";
import type { RenderFeature } from "./types";
import { packPaths } from "./pack-paths";

function feature(
  id: string,
  points: [number, number, number][],
  color: RenderFeature["color"],
  width: number,
  dash?: [number, number],
): RenderFeature {
  return {
    key: `LaneBoundary:0:${id}`,
    kind: "LaneBoundary",
    id,
    name: id,
    value: { id, geometry: points, type: "solid_line", crossingAllowed: false },
    layer: "boundaries",
    color,
    width,
    dash,
    lodPaths: { coarse: points, medium: points, fine: points },
  };
}

describe("PathLayer 二进制路径打包", () => {
  it("为每个顶点重复颜色、线宽和虚线参数", () => {
    const packed = packPaths([
      feature("solid", [[0, 0, 0], [1, 0, 0]], [222, 151, 36, 255], 2),
      feature("dashed", [[1, 0, 0], [2, 0, 0], [3, 0, 0]], [10, 20, 30, 255], 3, [8, 6]),
    ], "fine");

    expect([...packed.startIndices]).toEqual([0, 2, 5]);
    expect(packed.colors.length).toBe(5 * 4);
    expect(packed.widths.length).toBe(5);
    expect(packed.dashArrays.length).toBe(5 * 2);
    expect([...packed.colors.slice(0, 8)]).toEqual([
      222, 151, 36, 255, 222, 151, 36, 255,
    ]);
    expect([...packed.colors.slice(8)]).toEqual([
      10, 20, 30, 255, 10, 20, 30, 255, 10, 20, 30, 255,
    ]);
    expect([...packed.widths]).toEqual([2, 2, 3, 3, 3]);
    expect([...packed.dashArrays]).toEqual([0, 0, 0, 0, 8, 6, 8, 6, 8, 6]);
  });
});
