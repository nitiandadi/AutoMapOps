import type { LodLevel, PackedPathData, RenderFeature } from "./types";

/**
 * 按 PathLayer 二进制布局打包路径。
 *
 * getColor/getWidth/getDashArray 必须和 getPath 使用相同的逐顶点布局，
 * 不能只为每条路径写入一个值，否则长路径后半段会读取到错位属性。
 */
export function packPaths(features: readonly RenderFeature[], lod: LodLevel): PackedPathData {
  const vertexCount = features.reduce(
    (total, feature) => total + (feature.lodPaths?.[lod].length ?? 0),
    0,
  );
  const startIndices = new Uint32Array(features.length + 1);
  const positions = new Float32Array(vertexCount * 3);
  const colors = new Uint8Array(vertexCount * 4);
  const widths = new Float32Array(vertexCount);
  const dashArrays = new Float32Array(vertexCount * 2);

  let vertexOffset = 0;
  features.forEach((feature, featureIndex) => {
    startIndices[featureIndex] = vertexOffset;
    const dash = feature.dash ?? [0, 0];
    for (const point of feature.lodPaths?.[lod] ?? []) {
      const positionOffset = vertexOffset * 3;
      positions[positionOffset] = point[0];
      positions[positionOffset + 1] = point[1];
      positions[positionOffset + 2] = point[2];
      colors.set(feature.color, vertexOffset * 4);
      widths[vertexOffset] = feature.width;
      dashArrays.set(dash, vertexOffset * 2);
      vertexOffset += 1;
    }
  });
  startIndices[features.length] = vertexOffset;

  return { length: features.length, startIndices, positions, colors, widths, dashArrays };
}
