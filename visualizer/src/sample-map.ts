import type { MapData } from "./model";

export const sampleMap: MapData = {
  header: {
    mapId: "logistics_park_visualizer_demo",
    name: "物流园内部模型示意图",
    schemaVersion: "1.0",
  },
  roads: [
    {
      id: "road_main",
      name: "园区主路",
      referenceLine: [[0, 20, 0], [45, 20, 0], [85, 20, 0]],
      laneIds: ["lane_main_east", "lane_main_west"],
    },
    {
      id: "road_loading",
      name: "装卸区支路",
      referenceLine: [[45, 20, 0], [45, 48, 0], [45, 72, 0]],
      laneIds: ["lane_loading_in"],
    },
  ],
  lanes: [
    {
      id: "lane_main_east",
      roadId: "road_main",
      centerline: [[0, 18.2, 0], [85, 18.2, 0]],
      leftBoundaryId: "boundary_main_center",
      rightBoundaryId: "boundary_main_south",
      successorIds: ["lane_turn_connector"],
      direction: "along_reference_line",
      status: "open",
      widthM: 3.6,
      speedLimitMps: 5,
    },
    {
      id: "lane_main_west",
      roadId: "road_main",
      centerline: [[85, 21.8, 0], [0, 21.8, 0]],
      leftBoundaryId: "boundary_main_center",
      rightBoundaryId: "boundary_main_north",
      direction: "against_reference_line",
      status: "open",
      widthM: 3.6,
      speedLimitMps: 5,
    },
    {
      id: "lane_loading_in",
      roadId: "road_loading",
      centerline: [[43.2, 20, 0], [43.2, 70, 0]],
      leftBoundaryId: "boundary_loading_west",
      rightBoundaryId: "boundary_loading_east",
      predecessorIds: ["lane_turn_connector"],
      direction: "along_reference_line",
      status: "open",
      widthM: 3.6,
      speedLimitMps: 3,
    },
    {
      id: "lane_turn_connector",
      roadId: "road_loading",
      centerline: [[38, 18.2, 0], [41, 19, 0], [43.2, 23, 0]],
      predecessorIds: ["lane_main_east"],
      successorIds: ["lane_loading_in"],
      direction: "along_reference_line",
      status: "open",
      widthM: 3.6,
      speedLimitMps: 2,
    },
  ],
  laneBoundaries: [
    { id: "boundary_main_center", geometry: [[0, 20, 0], [85, 20, 0]], type: "dashed_line", crossingAllowed: true },
    { id: "boundary_main_south", geometry: [[0, 16.4, 0], [85, 16.4, 0]], type: "solid_line", crossingAllowed: false },
    { id: "boundary_main_north", geometry: [[0, 23.6, 0], [85, 23.6, 0]], type: "solid_line", crossingAllowed: false },
    { id: "boundary_loading_west", geometry: [[41.4, 24, 0], [41.4, 72, 0]], type: "solid_line", crossingAllowed: false },
    { id: "boundary_loading_east", geometry: [[45, 24, 0], [45, 72, 0]], type: "dashed_line", crossingAllowed: true },
  ],
  junctions: [
    { id: "junction_main_loading", name: "主路装卸区路口", connectionIds: ["connection_main_to_loading"] },
  ],
  laneConnections: [
    {
      id: "connection_main_to_loading",
      junctionId: "junction_main_loading",
      incomingLaneId: "lane_main_east",
      connectingLaneId: "lane_turn_connector",
      outgoingLaneId: "lane_loading_in",
      turnDirection: "left",
    },
  ],
  operationalAreas: [
    {
      id: "area_warehouse",
      name: "A 仓库",
      type: "warehouse",
      outline: [[52, 43, 0], [80, 43, 0], [80, 70, 0], [52, 70, 0], [52, 43, 0]],
    },
    {
      id: "area_loading",
      name: "装卸作业区",
      type: "loading_area",
      outline: [[46, 45, 0], [52, 45, 0], [52, 68, 0], [46, 68, 0], [46, 45, 0]],
    },
    {
      id: "area_charging",
      name: "充电区",
      type: "charging_area",
      outline: [[4, 28, 0], [18, 28, 0], [18, 40, 0], [4, 40, 0], [4, 28, 0]],
    },
  ],
  stations: [
    { id: "station_gate", name: "园区门岗", type: "gate", position: [2, 18.2, 0], accessLaneId: "lane_main_east" },
    { id: "station_loading_a1", name: "A1 月台", type: "loading_bay", position: [48, 54, 0], accessLaneId: "lane_loading_in" },
    { id: "station_charger_01", name: "1 号充电桩", type: "charging_point", position: [11, 34, 0], accessLaneId: "lane_main_west" },
  ],
  restrictedAreas: [
    {
      id: "restricted_narrow_passage",
      name: "窄通道",
      outline: [[39.5, 42, 0], [46, 42, 0], [46, 58, 0], [39.5, 58, 0], [39.5, 42, 0]],
      allowedVehicleProfileIds: ["vehicle_delivery_van"],
    },
  ],
  vehicleProfiles: [
    { id: "vehicle_delivery_van", name: "园区配送车", type: "delivery_van", widthM: 2.1, heightM: 2.8 },
    { id: "vehicle_truck_12m", name: "12 米物流卡车", type: "truck", widthM: 2.55, heightM: 4, lengthM: 12 },
  ],
};
