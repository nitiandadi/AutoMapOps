import { describe, expect, it } from "vitest";
import type { PathGeometry3d } from "../model";
import { evaluatePath, evaluateSegment, pathLength, simplifyPolyline, tessellatePath } from "./geometry";

describe("Canonical 曲线几何", () => {
  it("计算正负圆弧和 Z 插值", () => {
    const state = evaluateSegment({
      type: "circular_arc", start: [0, 0, 0], headingRad: 0,
      lengthM: Math.PI * 5, endZM: 2, curvaturePerM: 0.1,
    }, Math.PI * 5);
    expect(state.position[0]).toBeCloseTo(10, 8);
    expect(state.position[1]).toBeCloseTo(10, 8);
    expect(state.position[2]).toBeCloseTo(2, 8);
    expect(state.headingRad).toBeCloseTo(Math.PI / 2, 8);
  });

  it("与 C++ 共用的 Clothoid 黄金端点一致", () => {
    const state = evaluateSegment({
      type: "clothoid", start: [20, 0, 0.5], headingRad: 0, lengthM: 10,
      endZM: 1, startCurvaturePerM: 0, endCurvaturePerM: 0.05,
    }, 10);
    expect(state.position[0]).toBeCloseTo(29.9376805843, 7);
    expect(state.position[1]).toBeCloseTo(0.8296204854, 7);
    expect(state.headingRad).toBeCloseTo(0.25, 9);
  });

  it("按弧长求复合曲线中点并细分", () => {
    const path: PathGeometry3d = {
      type: "composite_curve",
      segments: [
        { type: "line", start: [0, 0, 0], headingRad: 0, lengthM: 10, endZM: 0 },
        { type: "circular_arc", start: [10, 0, 0], headingRad: 0, lengthM: 10, endZM: 0, curvaturePerM: 0.1 },
      ],
    };
    expect(pathLength(path)).toBe(20);
    expect(evaluatePath(path, 10).position).toEqual([10, 0, 0]);
    expect(tessellatePath(path, 0.01, 1).length).toBeGreaterThan(15);
  });

  it("折线简化保留首尾点", () => {
    const source: [number, number, number][] = [[0, 0, 0], [1, 0.001, 0], [2, 0, 0]];
    expect(simplifyPolyline(source, 0.01)).toEqual([[0, 0, 0], [2, 0, 0]]);
  });
});
