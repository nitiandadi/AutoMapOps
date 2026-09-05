/* 此文件由 npm run generate:types 生成，请勿手工修改。 */

export type Id = string;
export type PathGeometry3D = Polyline3D | CompositeCurve3D;
/**
 * @minItems 3
 * @maxItems 3
 */
export type Point3D = [number, number, number];
export type Polyline3D = Point3D[];
export type CurveSegment = LineSegment | CircularArcSegment | ClothoidSegment;
export type IdList = Id[];

export interface AutoMapOpsCanonicalMap11 {
  $schema?: string;
  header: Header;
  roads: Road[];
  lanes: Lane[];
  laneBoundaries: LaneBoundary[];
  junctions: Junction[];
  laneConnections: LaneConnection[];
  operationalAreas: OperationalArea[];
  stations: Station[];
  restrictedAreas: RestrictedArea[];
  vehicleProfiles: VehicleProfile[];
}
export interface Header {
  mapId: Id;
  name: string;
  schemaVersion: "1.0" | "1.1";
  coordinateReference: {
    geodeticDatum: "WGS84";
    origin: {
      longitudeDeg: number;
      latitudeDeg: number;
      altitudeM: number;
    };
    localFrame: "enu";
    linearUnit: "m";
    angleUnit: "rad";
  };
}
export interface Road {
  id: Id;
  name: string;
  referenceLine: PathGeometry3D;
  predecessorIds: IdList;
  successorIds: IdList;
  laneIds: IdList;
}
export interface CompositeCurve3D {
  type: "composite_curve";
  /**
   * @minItems 1
   */
  segments: [CurveSegment, ...CurveSegment[]];
}
export interface LineSegment {
  type: "line";
  start: Point3D;
  headingRad: number;
  lengthM: number;
  endZM: number;
}
export interface CircularArcSegment {
  type: "circular_arc";
  start: Point3D;
  headingRad: number;
  lengthM: number;
  endZM: number;
  curvaturePerM: number;
}
export interface ClothoidSegment {
  type: "clothoid";
  start: Point3D;
  headingRad: number;
  lengthM: number;
  endZM: number;
  startCurvaturePerM: number;
  endCurvaturePerM: number;
}
export interface Lane {
  id: Id;
  roadId: Id;
  centerline: PathGeometry3D;
  side: "left" | "right";
  orderFromReference: number;
  leftBoundaryId: Id;
  rightBoundaryId: Id;
  predecessorIds: IdList;
  successorIds: IdList;
  direction: "along_reference_line" | "against_reference_line";
  status: "open" | "closed";
  widthM: number;
  speedLimitMps: number;
}
export interface LaneBoundary {
  id: Id;
  geometry: PathGeometry3D;
  type: "unknown" | "dashed_line" | "solid_line" | "double_solid_line" | "curb" | "virtual_boundary";
  crossingAllowed: boolean;
}
export interface Junction {
  id: Id;
  name: string;
  connectionIds: IdList;
}
export interface LaneConnection {
  id: Id;
  junctionId: Id;
  incomingLaneId: Id;
  connectingLaneId: Id;
  outgoingLaneId: Id;
  turnDirection: "straight" | "left" | "right" | "u_turn";
}
export interface OperationalArea {
  id: Id;
  name: string;
  type: "unknown" | "warehouse" | "loading_area" | "parking_area" | "charging_area";
  outline: Polyline3D;
}
export interface Station {
  id: Id;
  name: string;
  type: "unknown" | "gate" | "loading_bay" | "parking_space" | "charging_point" | "waypoint";
  position: Point3D;
  accessLaneId: Id;
}
export interface RestrictedArea {
  id: Id;
  name: string;
  outline: Polyline3D;
  allowedVehicleProfileIds: IdList;
}
export interface VehicleProfile {
  id: Id;
  name: string;
  type: "passenger_car" | "delivery_van" | "truck";
  widthM: number;
  heightM: number | null;
  lengthM: number | null;
  minimumTurningRadiusM: number | null;
}
