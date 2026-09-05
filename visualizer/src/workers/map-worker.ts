/// <reference lib="webworker" />

import Ajv2020 from "ajv/dist/2020";
import Flatbush from "flatbush";
import canonicalSchema from "../../../schemas/canonical-map-1.1.schema.json";
import type { FeatureKind, MapData, MapObject, PathGeometry3d, Point3d } from "../model";
import { isCompositeCurve } from "../model";
import { boundsOfPoints, evaluatePath, pathMidpoint, simplifyPolyline, tessellatePath } from "../render/geometry";
import type { Bounds2d } from "../render/geometry";
import type { Color, LodLevel, PackedPathData, PackedPathGroup, PreparedMap, RenderFeature, WorkerRequest, WorkerResponse } from "../render/types";

const ajv = new Ajv2020({ allErrors: true, strict: false });
const validate = ajv.compile(canonicalSchema);
const lodOptions: Record<LodLevel, { error: number; maxLength: number }> = {
  coarse: { error: 0.5, maxLength: 25 },
  medium: { error: 0.125, maxLength: 10 },
  fine: { error: 0.015, maxLength: 5 },
};

function stableKey(kind: FeatureKind, index: number, id: string): string {
  return `${kind}:${index}:${id}`;
}

function colorFor(kind: FeatureKind, value: MapObject | Record<string, unknown>): Color {
  if (kind === "Lane" && "status" in value && value.status === "closed") return [210, 73, 62, 255];
  if (kind === "OperationalArea" && "type" in value) {
    const areaColors: Record<string, Color> = {
      warehouse: [78, 132, 91, 115],
      loading_area: [218, 143, 42, 120],
      parking_area: [59, 116, 169, 105],
      charging_area: [42, 146, 127, 110],
    };
    return areaColors[String(value.type)] ?? [91, 142, 104, 105];
  }
  if (kind === "Station" && "type" in value) {
    const stationColors: Record<string, Color> = {
      gate: [206, 108, 27, 255],
      loading_bay: [226, 148, 34, 255],
      parking_space: [55, 107, 165, 255],
      charging_point: [31, 143, 111, 255],
      waypoint: [116, 54, 158, 255],
    };
    return stationColors[String(value.type)] ?? [116, 54, 158, 255];
  }
  const colors: Partial<Record<FeatureKind, Color>> = {
    Road: [116, 129, 121, 220], Lane: [16, 132, 159, 255], LaneBoundary: [222, 151, 36, 255],
    OperationalArea: [91, 142, 104, 105], RestrictedArea: [190, 69, 58, 125],
    Station: [116, 54, 158, 255], LaneConnection: [220, 141, 31, 255], LaneTopology: [67, 88, 72, 210],
  };
  return colors[kind] ?? [90, 100, 94, 220];
}

function pathLods(geometry: PathGeometry3d): Record<LodLevel, Point3d[]> {
  const create = (level: LodLevel): Point3d[] => {
    const option = lodOptions[level];
    return isCompositeCurve(geometry)
      ? tessellatePath(geometry, option.error, option.maxLength)
      : simplifyPolyline(geometry, option.error);
  };
  return { coarse: create("coarse"), medium: create("medium"), fine: create("fine") };
}

function mergeBounds(target: Bounds2d | undefined, value: Bounds2d | undefined): Bounds2d | undefined {
  if (!value) return target;
  if (!target) return { ...value };
  target.minX = Math.min(target.minX, value.minX); target.minY = Math.min(target.minY, value.minY);
  target.maxX = Math.max(target.maxX, value.maxX); target.maxY = Math.max(target.maxY, value.maxY);
  return target;
}

function assertSchemaVersion(map: MapData): void {
  if (map.header.schemaVersion !== "1.0" && map.header.schemaVersion !== "1.1") {
    throw new Error(`$.header.schemaVersion：不支持的 Canonical Schema 版本 '${String(map.header.schemaVersion)}'`);
  }
  if (map.header.schemaVersion === "1.0") {
    const paths = [
      ...map.roads.map((value) => value.referenceLine),
      ...map.lanes.map((value) => value.centerline),
      ...map.laneBoundaries.map((value) => value.geometry),
    ];
    if (paths.some(isCompositeCurve)) throw new Error("Schema 1.0 的路径几何只能使用点列数组。");
  }
}

function validateMap(value: unknown): MapData {
  if (!validate(value)) {
    const first = validate.errors?.[0];
    throw new Error(`${first?.instancePath || "$"}：${first?.message || "不符合 Canonical JSON Schema"}`);
  }
  const map = value as unknown as MapData;
  assertSchemaVersion(map);
  return map;
}

function packPaths(features: RenderFeature[], lod: LodLevel): PackedPathData {
  const vertexCount = features.reduce((total, feature) => total + (feature.lodPaths?.[lod].length ?? 0), 0);
  const startIndices = new Uint32Array(features.length + 1);
  const positions = new Float32Array(vertexCount * 3);
  const colors = new Uint8Array(features.length * 4);
  const widths = new Float32Array(features.length);
  const dashArrays = new Float32Array(features.length * 2);
  let vertexOffset = 0;
  features.forEach((feature, featureIndex) => {
    startIndices[featureIndex] = vertexOffset;
    for (const point of feature.lodPaths?.[lod] ?? []) {
      const offset = vertexOffset * 3;
      positions[offset] = point[0]; positions[offset + 1] = point[1]; positions[offset + 2] = point[2];
      vertexOffset += 1;
    }
    colors.set(feature.color, featureIndex * 4);
    widths[featureIndex] = feature.width;
    dashArrays.set(feature.dash ?? [0, 0], featureIndex * 2);
  });
  startIndices[features.length] = vertexOffset;
  return { length: features.length, startIndices, positions, colors, widths, dashArrays };
}

function buildPathGroups(features: RenderFeature[]): Partial<Record<RenderFeature["layer"], PackedPathGroup>> {
  const groups: Partial<Record<RenderFeature["layer"], PackedPathGroup>> = {};
  const pathLayers = [...new Set(features.filter((feature) => feature.geometry).map((feature) => feature.layer))];
  pathLayers.forEach((layer) => {
    const layerFeatures = features.filter((feature) => feature.geometry && feature.layer === layer);
    groups[layer] = {
      featureKeys: layerFeatures.map((feature) => feature.key),
      lods: {
        coarse: packPaths(layerFeatures, "coarse"),
        medium: packPaths(layerFeatures, "medium"),
        fine: packPaths(layerFeatures, "fine"),
      },
    };
  });
  return groups;
}

function buildPreparedMap(map: MapData): PreparedMap {
  const features: RenderFeature[] = [];
  let mapBounds: Bounds2d | undefined;
  const addPath = (kind: FeatureKind, index: number, value: MapObject | Record<string, unknown>, geometry: PathGeometry3d, layer: RenderFeature["layer"], name: string, width: number, dash?: [number, number]): RenderFeature => {
    const lodPaths = pathLods(geometry);
    const bounds = boundsOfPoints(lodPaths.fine);
    mapBounds = mergeBounds(mapBounds, bounds);
    const feature: RenderFeature = {
      key: stableKey(kind, index, String(value.id)), kind, id: String(value.id), name,
      value, geometry, lodPaths, position: pathMidpoint(geometry), bounds, layer,
      color: colorFor(kind, value), width, dash,
    };
    features.push(feature);
    return feature;
  };
  const addObject = (kind: FeatureKind, index: number, value: MapObject, layer: RenderFeature["layer"], position?: Point3d): RenderFeature => {
    const name = "name" in value && typeof value.name === "string" ? value.name : value.id;
    const bounds = position ? boundsOfPoints([position]) : undefined;
    mapBounds = mergeBounds(mapBounds, bounds);
    const feature: RenderFeature = {
      key: stableKey(kind, index, value.id), kind, id: value.id, name, value,
      layer, position, bounds, color: colorFor(kind, value), width: 1,
    };
    features.push(feature);
    return feature;
  };

  map.roads.forEach((road, index) => {
    const feature = addPath("Road", index, road, road.referenceLine, "roads", road.name || road.id, 2, [2, 5]);
    feature.labelPriority = 70;
  });
  map.lanes.forEach((lane, index) => {
    const feature = addPath("Lane", index, lane, lane.centerline, "lanes", lane.id, 3.5);
    feature.labelPriority = 40;
  });
  map.laneBoundaries.forEach((boundary, index) => {
    addPath("LaneBoundary", index, boundary, boundary.geometry, "boundaries", boundary.id, boundary.type === "curb" ? 4 : 2, boundary.type === "dashed_line" ? [8, 6] : undefined);
  });
  map.operationalAreas.forEach((area, index) => {
    const feature = addObject("OperationalArea", index, area, "areas", pathMidpoint(area.outline));
    feature.polygon = area.outline; feature.bounds = boundsOfPoints(area.outline); feature.labelPriority = 60;
    mapBounds = mergeBounds(mapBounds, feature.bounds);
  });
  map.restrictedAreas.forEach((area, index) => {
    const feature = addObject("RestrictedArea", index, area, "restricted", pathMidpoint(area.outline));
    feature.polygon = area.outline; feature.bounds = boundsOfPoints(area.outline); feature.labelPriority = 55;
    mapBounds = mergeBounds(mapBounds, feature.bounds);
  });
  map.stations.forEach((station, index) => {
    const feature = addObject("Station", index, station, "stations", station.position);
    feature.labelPriority = 90;
  });
  map.junctions.forEach((junction, index) => {
    const connection = map.laneConnections.find((item) => item.junctionId === junction.id);
    const lane = connection ? map.lanes.find((item) => item.id === connection.connectingLaneId) : undefined;
    const feature = addObject("Junction", index, junction, "junctions", lane ? pathMidpoint(lane.centerline) : undefined);
    feature.labelPriority = 85;
  });
  map.laneConnections.forEach((connection, index) => {
    const lane = map.lanes.find((item) => item.id === connection.connectingLaneId);
    addObject("LaneConnection", index, connection, "junctions", lane ? pathMidpoint(lane.centerline) : undefined);
  });
  map.vehicleProfiles.forEach((profile, index) => addObject("VehicleProfile", index, profile, "labels"));

  const lanesById = new Map(map.lanes.map((lane) => [lane.id, lane]));
  let topologyIndex = 0;
  map.lanes.forEach((lane) => lane.successorIds.forEach((successorId) => {
    const target = lanesById.get(successorId);
    if (!target) return;
    const from = evaluatePath(lane.centerline, Number.POSITIVE_INFINITY).position;
    const to = evaluatePath(target.centerline, 0).position;
    addPath(
      "LaneTopology", topologyIndex, { id: `${lane.id} → ${successorId}`, from: lane.id, to: successorId },
      [from, to], "topology", `${lane.id} → ${successorId}`, 1.5, [4, 4],
    );
    topologyIndex += 1;
  }));

  const bounds = mapBounds ?? { minX: -1, minY: -1, maxX: 1, maxY: 1 };
  const origin: [number, number] = [(bounds.minX + bounds.maxX) * 0.5, (bounds.minY + bounds.maxY) * 0.5];
  // 当前查看器是二维正交视图。原始高程保留在 map/value/geometry 中供检查器与精确计算使用，
  // GPU 坐标统一压平到 Z=0，避免正高程被 OrthographicView 的近裁剪面截断。
  const shiftPoint = (point: Point3d): Point3d => [point[0] - origin[0], point[1] - origin[1], 0];
  features.forEach((feature) => {
    if (feature.lodPaths) {
      feature.lodPaths = {
        coarse: feature.lodPaths.coarse.map(shiftPoint),
        medium: feature.lodPaths.medium.map(shiftPoint),
        fine: feature.lodPaths.fine.map(shiftPoint),
      };
    }
    if (feature.polygon) feature.polygon = feature.polygon.map(shiftPoint);
    if (feature.position) feature.position = shiftPoint(feature.position);
    if (feature.bounds) {
      feature.bounds = {
        minX: feature.bounds.minX - origin[0], minY: feature.bounds.minY - origin[1],
        maxX: feature.bounds.maxX - origin[0], maxY: feature.bounds.maxY - origin[1],
      };
    }
  });
  const shiftedBounds = {
    minX: bounds.minX - origin[0], minY: bounds.minY - origin[1],
    maxX: bounds.maxX - origin[0], maxY: bounds.maxY - origin[1],
  };
  const spatialFeatures = features.filter((feature) => feature.bounds);
  const index = spatialFeatures.length > 0 ? new Flatbush(spatialFeatures.length) : undefined;
  spatialFeatures.forEach((feature) => index!.add(feature.bounds!.minX, feature.bounds!.minY, feature.bounds!.maxX, feature.bounds!.maxY));
  index?.finish();
  const renderedVertexCount = features.reduce((total, feature) => total + (feature.lodPaths?.fine.length ?? feature.polygon?.length ?? (feature.position ? 1 : 0)), 0);
  const pathGroups = buildPathGroups(features);
  return {
    map, features, pathGroups, bounds: shiftedBounds, origin,
    spatialIndexData: index?.data instanceof ArrayBuffer ? index.data : undefined,
    spatialFeatureKeys: spatialFeatures.map((feature) => feature.key),
    stats: { objectCount: features.length, renderedVertexCount, pathCount: features.filter((feature) => feature.geometry).length },
  };
}

self.onmessage = (event: MessageEvent<WorkerRequest>): void => {
  const request = event.data;
  try {
    const raw = request.text === undefined ? request.map : JSON.parse(request.text);
    const result = buildPreparedMap(validateMap(raw));
    const response: WorkerResponse = { requestId: request.requestId, ok: true, result };
    const transfers: Transferable[] = result.spatialIndexData ? [result.spatialIndexData] : [];
    Object.values(result.pathGroups).forEach((group) => Object.values(group.lods).forEach((lod) => {
      transfers.push(lod.startIndices.buffer, lod.positions.buffer, lod.colors.buffer, lod.widths.buffer, lod.dashArrays.buffer);
    }));
    self.postMessage(response, { transfer: transfers });
  } catch (error) {
    const response: WorkerResponse = {
      requestId: request.requestId,
      ok: false,
      error: error instanceof Error ? error.message : "无法读取地图数据",
    };
    self.postMessage(response);
  }
};
