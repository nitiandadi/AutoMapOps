import { COORDINATE_SYSTEM } from "@deck.gl/core";
import type { Layer } from "@deck.gl/core";
import { PolygonLayer, ScatterplotLayer, TextLayer } from "@deck.gl/layers";
import { CollisionFilterExtension } from "@deck.gl/extensions";
import type { CollisionFilterExtensionProps } from "@deck.gl/extensions";
import type { TextLayerProps } from "@deck.gl/layers";
import type { LayerName } from "../model";
import type { LodLevel, PreparedMap, RenderFeature } from "./types";
import { CanonicalCurveLayer } from "./CanonicalCurveLayer";

export interface LayerBuildOptions {
  prepared: PreparedMap;
  visibility: Record<LayerName, boolean>;
  selectedKey: string;
  lod: LodLevel;
  visibleLabelKeys?: Set<string>;
  minimumLabelPriority?: number;
}

function alpha(color: RenderFeature["color"], value: number): RenderFeature["color"] {
  return [color[0], color[1], color[2], value];
}

export function buildLayers(options: LayerBuildOptions): Layer[] {
  const { prepared, visibility, selectedKey, lod } = options;
  const layers: Layer[] = [];
  Object.entries(prepared.pathGroups).forEach(([layerName, packed]) => {
    const layer = layerName as LayerName;
    if (!visibility[layer]) return;
    const keys = new Set(packed.featureKeys);
    const features = prepared.features.filter((feature) => keys.has(feature.key));
    layers.push(new CanonicalCurveLayer({ id: `canonical-paths-${layer}`, features, packed, lod, selectedKey }));
  });

  const polygonFeatures = prepared.features.filter((feature) => feature.polygon && visibility[feature.layer]);
  if (polygonFeatures.length > 0) {
    layers.push(new PolygonLayer<RenderFeature>({
      id: "canonical-polygons", data: polygonFeatures,
      coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      getPolygon: (feature) => feature.polygon!,
      getFillColor: (feature) => feature.key === selectedKey ? [225, 54, 44, 105] : feature.color,
      getLineColor: (feature) => feature.key === selectedKey ? [225, 54, 44, 255] : alpha(feature.color, 255),
      getLineWidth: (feature) => feature.key === selectedKey ? 4 : 2,
      lineWidthUnits: "pixels", stroked: true, filled: true, pickable: true,
    }));
  }

  const pointFeatures = prepared.features.filter((feature) => feature.position
    && ["Station", "Junction", "LaneConnection"].includes(feature.kind)
    && visibility[feature.layer]);
  if (pointFeatures.length > 0) {
    layers.push(new ScatterplotLayer<RenderFeature>({
      id: "canonical-points", data: pointFeatures,
      coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      getPosition: (feature) => feature.position!,
      getRadius: (feature) => feature.key === selectedKey ? 9 : 6,
      radiusUnits: "pixels", radiusMinPixels: 6,
      getFillColor: (feature) => feature.key === selectedKey ? [225, 54, 44, 255] : feature.color,
      getLineColor: [255, 255, 255, 255], getLineWidth: 2,
      lineWidthUnits: "pixels", stroked: true, pickable: true,
    }));
  }

  if (visibility.labels) {
    const labels = prepared.features.filter((feature) => feature.position && feature.labelPriority
      && (feature.labelPriority ?? 0) >= (options.minimumLabelPriority ?? 0)
      && (!options.visibleLabelKeys || options.visibleLabelKeys.has(feature.key)));
    const labelProps: TextLayerProps<RenderFeature> & CollisionFilterExtensionProps<RenderFeature> = {
      id: "canonical-labels", data: labels.filter((feature) => feature.key !== selectedKey),
      coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      getPosition: (feature) => feature.position!, getText: (feature) => feature.name,
      getSize: 12, sizeUnits: "pixels", getColor: [36, 49, 41, 255],
      getPixelOffset: [7, -8], getTextAnchor: "start", getAlignmentBaseline: "bottom",
      getCollisionPriority: (feature: RenderFeature) => feature.labelPriority ?? 0,
      collisionEnabled: true, collisionGroup: "canonical-labels",
      fontFamily: 'Inter, "Segoe UI", "Microsoft YaHei", sans-serif',
      outlineWidth: 2, outlineColor: [245, 247, 242, 255],
      extensions: [new CollisionFilterExtension()], pickable: false,
    };
    layers.push(new TextLayer<RenderFeature>(labelProps));
    const selected = prepared.features.filter((feature) => feature.key === selectedKey && feature.position);
    if (selected.length > 0) {
      layers.push(new TextLayer<RenderFeature>({
        id: "selected-label", data: selected,
        coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
        getPosition: (feature) => feature.position!, getText: (feature) => feature.name,
        getSize: 13, sizeUnits: "pixels", getColor: [139, 28, 21, 255],
        getPixelOffset: [8, -10], getTextAnchor: "start", getAlignmentBaseline: "bottom",
        fontWeight: 700, outlineWidth: 3, outlineColor: [255, 255, 255, 255], pickable: false,
      }));
    }
  }
  return layers;
}
