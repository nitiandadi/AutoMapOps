import { describe, expect, it } from "vitest";
import { simplifyPolyline } from "../src/render/geometry";
import type { Point3d } from "../src/model";

describe("10 万对象 / 100 万顶点几何基准", () => {
  it("构建并简化合成折线数据", () => {
    const startedAt = performance.now();
    let outputVertices = 0;
    for (let objectIndex = 0; objectIndex < 100_000; objectIndex += 1) {
      const points: Point3d[] = [];
      for (let pointIndex = 0; pointIndex < 10; pointIndex += 1) {
        points.push([pointIndex, objectIndex * 0.01 + Math.sin(pointIndex) * 0.001, 0]);
      }
      outputVertices += simplifyPolyline(points, 0.01).length;
    }
    const elapsedMs = performance.now() - startedAt;
    console.info(JSON.stringify({ objects: 100_000, inputVertices: 1_000_000, outputVertices, elapsedMs }));
    expect(outputVertices).toBeGreaterThanOrEqual(200_000);
  }, 30_000);
});
