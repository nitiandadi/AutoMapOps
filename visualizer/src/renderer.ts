import type {
  Lane,
  MapData,
  Point2d,
  PointLike,
} from "./model";
import { point2d } from "./model";

const SVG_NS = "http://www.w3.org/2000/svg";
const CANVAS_WIDTH = 1200;
const CANVAS_HEIGHT = 800;
const PADDING = 70;

export type LayerName =
  | "areas"
  | "restricted"
  | "roads"
  | "boundaries"
  | "lanes"
  | "topology"
  | "junctions"
  | "stations"
  | "labels";

export interface SelectedObject {
  kind: string;
  value: object;
}

interface Projection {
  point(value: PointLike): Point2d;
}

function svgElement<K extends keyof SVGElementTagNameMap>(
  name: K,
  attributes: Record<string, string> = {},
): SVGElementTagNameMap[K] {
  const element = document.createElementNS(SVG_NS, name);
  Object.entries(attributes).forEach(([key, value]) => element.setAttribute(key, value));
  return element;
}

function collectPoints(map: MapData): Point2d[] {
  const points: Point2d[] = [];
  const add = (values?: PointLike[]) => values?.forEach((value) => points.push(point2d(value)));
  map.roads?.forEach((road) => add(road.referenceLine));
  map.lanes?.forEach((lane) => add(lane.centerline));
  map.laneBoundaries?.forEach((boundary) => add(boundary.geometry));
  map.operationalAreas?.forEach((area) => add(area.outline));
  map.restrictedAreas?.forEach((area) => add(area.outline));
  map.stations?.forEach((station) => {
    if (station.position) points.push(point2d(station.position));
  });
  return points;
}

function createProjection(map: MapData): Projection {
  const points = collectPoints(map);
  if (points.length === 0) {
    return { point: (value) => point2d(value) };
  }

  const xs = points.map(({ x }) => x);
  const ys = points.map(({ y }) => y);
  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);
  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);
  const spanX = Math.max(maxX - minX, 1);
  const spanY = Math.max(maxY - minY, 1);
  const scale = Math.min(
    (CANVAS_WIDTH - PADDING * 2) / spanX,
    (CANVAS_HEIGHT - PADDING * 2) / spanY,
  );
  const contentWidth = spanX * scale;
  const contentHeight = spanY * scale;
  const offsetX = (CANVAS_WIDTH - contentWidth) / 2;
  const offsetY = (CANVAS_HEIGHT - contentHeight) / 2;

  return {
    point(value: PointLike): Point2d {
      const { x, y } = point2d(value);
      return {
        x: offsetX + (x - minX) * scale,
        y: CANVAS_HEIGHT - offsetY - (y - minY) * scale,
      };
    },
  };
}

function pathData(points: PointLike[] | undefined, projection: Projection): string {
  if (!points?.length) return "";
  return points
    .map((point, index) => {
      const projected = projection.point(point);
      return `${index === 0 ? "M" : "L"} ${projected.x.toFixed(2)} ${projected.y.toFixed(2)}`;
    })
    .join(" ");
}

function midpoint(points: PointLike[] | undefined, projection: Projection): Point2d | undefined {
  if (!points?.length) return undefined;
  return projection.point(points[Math.floor(points.length / 2)]);
}

function endpoint(lane: Lane | undefined, first: boolean): PointLike | undefined {
  if (!lane?.centerline?.length) return undefined;
  return first ? lane.centerline[0] : lane.centerline[lane.centerline.length - 1];
}

function attachObject(element: SVGElement, kind: string, value: object): void {
  element.dataset.objectKind = kind;
  element.dataset.objectId = "id" in value && typeof value.id === "string" ? value.id : "";
}

function addLabel(group: SVGGElement, position: Point2d | undefined, text: string): void {
  if (!position) return;
  const label = svgElement("text", {
    x: (position.x + 7).toFixed(2),
    y: (position.y - 7).toFixed(2),
    class: "map-label",
  });
  label.textContent = text;
  group.append(label);
}

export class MapRenderer {
  private readonly groups = new Map<LayerName, SVGGElement>();
  private viewBox = { x: 0, y: 0, width: CANVAS_WIDTH, height: CANVAS_HEIGHT };
  private dragOrigin?: { clientX: number; clientY: number; x: number; y: number };

  constructor(
    private readonly svg: SVGSVGElement,
    private readonly onSelect: (selected: SelectedObject) => void,
  ) {
    this.installViewportControls();
  }

  render(map: MapData): void {
    this.svg.replaceChildren();
    this.groups.clear();
    this.installDefinitions();

    const projection = createProjection(map);
    const layers: LayerName[] = [
      "areas", "restricted", "roads", "boundaries", "lanes",
      "topology", "junctions", "stations", "labels",
    ];
    layers.forEach((name) => {
      const group = svgElement("g", { class: `layer layer-${name}` });
      this.groups.set(name, group);
      this.svg.append(group);
    });

    const labels = this.groups.get("labels")!;

    map.operationalAreas?.forEach((area) => {
      const path = svgElement("path", {
        d: `${pathData(area.outline, projection)} Z`,
        class: `map-object operational-area area-${area.type || "unknown"}`,
      });
      attachObject(path, "OperationalArea", area);
      this.groups.get("areas")!.append(path);
      addLabel(labels, midpoint(area.outline, projection), area.name || area.id);
    });

    map.restrictedAreas?.forEach((area) => {
      const path = svgElement("path", {
        d: `${pathData(area.outline, projection)} Z`,
        class: "map-object restricted-area",
      });
      attachObject(path, "RestrictedArea", area);
      this.groups.get("restricted")!.append(path);
      addLabel(labels, midpoint(area.outline, projection), area.name || area.id);
    });

    map.roads?.forEach((road) => {
      const path = svgElement("path", {
        d: pathData(road.referenceLine, projection),
        class: "map-object road-reference",
      });
      attachObject(path, "Road", road);
      this.groups.get("roads")!.append(path);
      addLabel(labels, midpoint(road.referenceLine, projection), road.name || road.id);
    });

    map.laneBoundaries?.forEach((boundary) => {
      const path = svgElement("path", {
        d: pathData(boundary.geometry, projection),
        class: `map-object lane-boundary boundary-${boundary.type || "unknown"}`,
      });
      attachObject(path, "LaneBoundary", boundary);
      this.groups.get("boundaries")!.append(path);
    });

    map.lanes?.forEach((lane) => {
      const path = svgElement("path", {
        d: pathData(lane.centerline, projection),
        class: `map-object lane-centerline lane-${lane.status || "unknown"}`,
      });
      attachObject(path, "Lane", lane);
      this.groups.get("lanes")!.append(path);
      addLabel(labels, midpoint(lane.centerline, projection), lane.id);
    });

    const lanesById = new Map(map.lanes?.map((lane) => [lane.id, lane]) || []);
    map.lanes?.forEach((lane) => {
      lane.successorIds?.forEach((successorId) => {
        const from = endpoint(lane, false);
        const to = endpoint(lanesById.get(successorId), true);
        if (!from || !to) return;
        const a = projection.point(from);
        const b = projection.point(to);
        const line = svgElement("line", {
          x1: a.x.toFixed(2), y1: a.y.toFixed(2),
          x2: b.x.toFixed(2), y2: b.y.toFixed(2),
          class: "topology-link",
          "marker-end": "url(#arrowhead)",
        });
        attachObject(line, "LaneTopology", { id: `${lane.id} → ${successorId}`, from: lane.id, to: successorId });
        this.groups.get("topology")!.append(line);
      });
    });

    map.laneConnections?.forEach((connection) => {
      const connector = lanesById.get(connection.connectingLaneId || "");
      const position = midpoint(connector?.centerline, projection);
      if (!position) return;
      const circle = svgElement("circle", {
        cx: position.x.toFixed(2), cy: position.y.toFixed(2), r: "10",
        class: "map-object junction-node",
      });
      attachObject(circle, "LaneConnection", connection);
      this.groups.get("junctions")!.append(circle);
    });

    map.stations?.forEach((station) => {
      if (!station.position) return;
      const position = projection.point(station.position);
      const marker = svgElement("circle", {
        cx: position.x.toFixed(2), cy: position.y.toFixed(2), r: "8",
        class: `map-object station station-${station.type || "unknown"}`,
      });
      attachObject(marker, "Station", station);
      this.groups.get("stations")!.append(marker);
      addLabel(labels, position, station.name || station.id);
    });

    this.svg.onclick = (event) => {
      const target = (event.target as Element).closest("[data-object-id]") as SVGElement | null;
      if (!target) return;
      const kind = target.dataset.objectKind || "Unknown";
      const id = target.dataset.objectId || "";
      const value = this.findObject(map, kind, id);
      if (value) this.onSelect({ kind, value });
      this.svg.querySelectorAll(".selected").forEach((item) => item.classList.remove("selected"));
      target.classList.add("selected");
    };

    this.resetView();
  }

  setLayerVisible(layer: LayerName, visible: boolean): void {
    this.groups.get(layer)?.classList.toggle("hidden", !visible);
  }

  resetView(): void {
    this.viewBox = { x: 0, y: 0, width: CANVAS_WIDTH, height: CANVAS_HEIGHT };
    this.applyViewBox();
  }

  private installDefinitions(): void {
    const definitions = svgElement("defs");
    const marker = svgElement("marker", {
      id: "arrowhead", markerWidth: "8", markerHeight: "8",
      refX: "7", refY: "4", orient: "auto", markerUnits: "strokeWidth",
    });
    marker.append(svgElement("path", { d: "M 0 0 L 8 4 L 0 8 Z", class: "arrowhead" }));
    definitions.append(marker);
    this.svg.append(definitions);
  }

  private installViewportControls(): void {
    this.svg.addEventListener("wheel", (event) => {
      event.preventDefault();
      const rect = this.svg.getBoundingClientRect();
      const ratioX = (event.clientX - rect.left) / rect.width;
      const ratioY = (event.clientY - rect.top) / rect.height;
      const factor = event.deltaY > 0 ? 1.15 : 0.87;
      const nextWidth = Math.min(Math.max(this.viewBox.width * factor, 120), 5000);
      const nextHeight = nextWidth * (CANVAS_HEIGHT / CANVAS_WIDTH);
      this.viewBox.x += (this.viewBox.width - nextWidth) * ratioX;
      this.viewBox.y += (this.viewBox.height - nextHeight) * ratioY;
      this.viewBox.width = nextWidth;
      this.viewBox.height = nextHeight;
      this.applyViewBox();
    }, { passive: false });

    this.svg.addEventListener("pointerdown", (event) => {
      if (event.button !== 0) return;
      this.svg.setPointerCapture(event.pointerId);
      this.dragOrigin = { clientX: event.clientX, clientY: event.clientY, x: this.viewBox.x, y: this.viewBox.y };
      this.svg.classList.add("dragging");
    });
    this.svg.addEventListener("pointermove", (event) => {
      if (!this.dragOrigin) return;
      const rect = this.svg.getBoundingClientRect();
      this.viewBox.x = this.dragOrigin.x - (event.clientX - this.dragOrigin.clientX) * this.viewBox.width / rect.width;
      this.viewBox.y = this.dragOrigin.y - (event.clientY - this.dragOrigin.clientY) * this.viewBox.height / rect.height;
      this.applyViewBox();
    });
    const stopDragging = (): void => {
      this.dragOrigin = undefined;
      this.svg.classList.remove("dragging");
    };
    this.svg.addEventListener("pointerup", stopDragging);
    this.svg.addEventListener("pointercancel", stopDragging);
  }

  private applyViewBox(): void {
    const { x, y, width, height } = this.viewBox;
    this.svg.setAttribute("viewBox", `${x} ${y} ${width} ${height}`);
  }

  private findObject(map: MapData, kind: string, id: string): object | undefined {
    const collections: Record<string, object[] | undefined> = {
      Road: map.roads,
      Lane: map.lanes,
      LaneBoundary: map.laneBoundaries,
      Junction: map.junctions,
      LaneConnection: map.laneConnections,
      OperationalArea: map.operationalAreas,
      Station: map.stations,
      RestrictedArea: map.restrictedAreas,
      VehicleProfile: map.vehicleProfiles,
    };
    if (kind === "LaneTopology") return { id };
    return collections[kind]?.find((value) => "id" in value && value.id === id);
  }
}
