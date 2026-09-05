import type { Bounds2d } from "./geometry";
import type { LayerName, MapData, PathGeometry3d, Point3d, SelectedObject } from "../model";

export type Color = [number, number, number, number];
export type LodLevel = "coarse" | "medium" | "fine";

export interface RenderFeature extends SelectedObject {
  layer: LayerName;
  name: string;
  geometry?: PathGeometry3d;
  lodPaths?: Record<LodLevel, Point3d[]>;
  polygon?: Point3d[];
  position?: Point3d;
  bounds?: Bounds2d;
  color: Color;
  width: number;
  dash?: [number, number];
  labelPriority?: number;
}

export interface PreparedMap {
  map: MapData;
  features: RenderFeature[];
  pathGroups: Partial<Record<LayerName, PackedPathGroup>>;
  bounds: Bounds2d;
  origin: [number, number];
  spatialIndexData?: ArrayBuffer;
  spatialFeatureKeys: string[];
  stats: {
    objectCount: number;
    renderedVertexCount: number;
    pathCount: number;
  };
}

export interface PackedPathData {
  length: number;
  startIndices: Uint32Array;
  positions: Float32Array;
  colors: Uint8Array;
  widths: Float32Array;
  dashArrays: Float32Array;
}

export interface PackedPathGroup {
  featureKeys: string[];
  lods: Record<LodLevel, PackedPathData>;
}

export interface WorkerRequest {
  requestId: number;
  source: string;
  text?: string;
  map?: MapData;
}

export type WorkerResponse =
  | { requestId: number; ok: true; result: PreparedMap }
  | { requestId: number; ok: false; error: string };
