import { CompositeLayer, COORDINATE_SYSTEM } from "@deck.gl/core";
import type { CompositeLayerProps, DefaultProps } from "@deck.gl/core";
import type { GetPickingInfoParams, PickingInfo } from "@deck.gl/core";
import { PathLayer } from "@deck.gl/layers";
import { PathStyleExtension } from "@deck.gl/extensions";
import type { LodLevel, PackedPathGroup, RenderFeature } from "./types";

export interface CanonicalCurveLayerProps extends CompositeLayerProps {
  features: RenderFeature[];
  packed: PackedPathGroup;
  lod: LodLevel;
  selectedKey: string;
}

export class CanonicalCurveLayer extends CompositeLayer<CanonicalCurveLayerProps> {
  static layerName = "CanonicalCurveLayer";
  static defaultProps: DefaultProps<CanonicalCurveLayerProps> = {
    features: { type: "array", value: [], compare: true },
    lod: "medium",
    selectedKey: "",
  };

  getPickingInfo({ info }: GetPickingInfoParams): PickingInfo {
    if (info.index >= 0) info.object = this.props.features[info.index];
    return info;
  }

  renderLayers() {
    const { features, packed, lod, selectedKey } = this.props;
    const path = (feature: RenderFeature) => feature.lodPaths?.[lod] ?? [];
    const binary = packed.lods[lod];
    const binaryData = {
      length: binary.length,
      startIndices: binary.startIndices,
      attributes: {
        getPath: { value: binary.positions, size: 3 },
        getColor: { value: binary.colors, size: 4 },
        getWidth: { value: binary.widths, size: 1 },
        getDashArray: { value: binary.dashArrays, size: 2 },
      },
    };
    const hitData = {
      length: binary.length,
      startIndices: binary.startIndices,
      attributes: { getPath: { value: binary.positions, size: 3 } },
    };
    const visual = new PathLayer<RenderFeature>(this.getSubLayerProps({
      id: "visual",
      data: binaryData,
      coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      positionFormat: "XYZ",
      getPath: path,
      getColor: [255, 255, 255, 255],
      getWidth: 1,
      widthUnits: "pixels",
      widthMinPixels: 1,
      rounded: true,
      jointRounded: true,
      getDashArray: [0, 0],
      dashJustified: true,
      extensions: [new PathStyleExtension({ dash: true })],
      pickable: false,
    }));
    const hit = new PathLayer<RenderFeature>(this.getSubLayerProps({
      id: "hit",
      data: hitData,
      coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      positionFormat: "XYZ",
      getPath: path,
      getColor: [0, 0, 0, 1],
      getWidth: 10,
      widthUnits: "pixels",
      widthMinPixels: 10,
      opacity: 0.01,
      pickable: true,
    }));
    const selected = features.filter((feature) => feature.key === selectedKey);
    const highlight = selected.length === 0 ? null : new PathLayer<RenderFeature>(this.getSubLayerProps({
      id: "selection", data: selected, coordinateSystem: COORDINATE_SYSTEM.CARTESIAN,
      getPath: path, getColor: [225, 54, 44, 255], getWidth: 7,
      widthUnits: "pixels", rounded: true, jointRounded: true, pickable: false,
    }));
    return [visual, hit, highlight];
  }
}
