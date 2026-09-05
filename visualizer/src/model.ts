import type {
  AutoMapOpsCanonicalMap11,
  CircularArcSegment,
  ClothoidSegment,
  CompositeCurve3D,
  CurveSegment,
  Junction,
  Lane,
  LaneBoundary,
  LaneConnection,
  LineSegment,
  OperationalArea,
  PathGeometry3D,
  Point3D,
  RestrictedArea,
  Road,
  Station,
  VehicleProfile,
} from "./generated/canonical-map.generated";

export type MapData = AutoMapOpsCanonicalMap11;
export type Point3d = Point3D;
export type Polyline3d = Point3D[];
export type PathGeometry3d = PathGeometry3D;
export type CompositeCurve3d = CompositeCurve3D;
export type CurveSegment3d = CurveSegment;
export type LineCurveSegment3d = LineSegment;
export type CircularArcCurveSegment3d = CircularArcSegment;
export type ClothoidCurveSegment3d = ClothoidSegment;

export type MapObject = Road | Lane | LaneBoundary | Junction | LaneConnection
  | OperationalArea | Station | RestrictedArea | VehicleProfile;

export type FeatureKind = "Road" | "Lane" | "LaneBoundary" | "Junction"
  | "LaneConnection" | "OperationalArea" | "Station" | "RestrictedArea"
  | "VehicleProfile" | "LaneTopology";

export type LayerName = "areas" | "restricted" | "roads" | "boundaries"
  | "lanes" | "topology" | "junctions" | "stations" | "labels";

export interface SelectedObject {
  key: string;
  kind: FeatureKind;
  id: string;
  value: MapObject | Record<string, unknown>;
}

export function mapName(map: MapData): string {
  return map.header.name || "未命名地图";
}

export function mapId(map: MapData): string {
  return map.header.mapId || "unknown_map";
}

export function isCompositeCurve(geometry: PathGeometry3d): geometry is CompositeCurve3d {
  return !Array.isArray(geometry) && geometry.type === "composite_curve";
}
