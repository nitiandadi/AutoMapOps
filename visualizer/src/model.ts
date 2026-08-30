export type PointLike = [number, number, number?] | { x: number; y: number; z?: number };

export interface MapHeader {
  mapId?: string | { value?: string };
  name?: string;
  schemaVersion?: string;
}

export interface Road {
  id: string;
  name?: string;
  referenceLine?: PointLike[];
  predecessorIds?: string[];
  successorIds?: string[];
  laneIds?: string[];
}

export interface Lane {
  id: string;
  roadId?: string;
  centerline?: PointLike[];
  leftBoundaryId?: string;
  rightBoundaryId?: string;
  predecessorIds?: string[];
  successorIds?: string[];
  direction?: string;
  status?: string;
  widthM?: number;
  speedLimitMps?: number;
}

export interface LaneBoundary {
  id: string;
  geometry?: PointLike[];
  type?: string;
  crossingAllowed?: boolean;
}

export interface Junction {
  id: string;
  name?: string;
  connectionIds?: string[];
}

export interface LaneConnection {
  id: string;
  junctionId?: string;
  incomingLaneId?: string;
  connectingLaneId?: string;
  outgoingLaneId?: string;
  turnDirection?: string;
}

export interface OperationalArea {
  id: string;
  name?: string;
  type?: string;
  outline?: PointLike[];
}

export interface Station {
  id: string;
  name?: string;
  type?: string;
  position?: PointLike;
  accessLaneId?: string;
}

export interface RestrictedArea {
  id: string;
  name?: string;
  outline?: PointLike[];
  allowedVehicleProfileIds?: string[];
}

export interface VehicleProfile {
  id: string;
  name?: string;
  type?: string;
  widthM?: number;
  heightM?: number;
  lengthM?: number;
  minimumTurningRadiusM?: number;
}

export interface MapData {
  header?: MapHeader;
  roads?: Road[];
  lanes?: Lane[];
  laneBoundaries?: LaneBoundary[];
  junctions?: Junction[];
  laneConnections?: LaneConnection[];
  operationalAreas?: OperationalArea[];
  stations?: Station[];
  restrictedAreas?: RestrictedArea[];
  vehicleProfiles?: VehicleProfile[];
}

export interface Point2d {
  x: number;
  y: number;
}

export function point2d(point: PointLike): Point2d {
  return Array.isArray(point) ? { x: point[0], y: point[1] } : { x: point.x, y: point.y };
}

export function mapName(map: MapData): string {
  return map.header?.name || "未命名地图";
}

export function mapId(map: MapData): string {
  const value = map.header?.mapId;
  if (typeof value === "string") return value;
  return value?.value || "unknown_map";
}
