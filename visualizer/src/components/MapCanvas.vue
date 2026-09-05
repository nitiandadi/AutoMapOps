<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from "vue";
import { Deck, OrthographicView } from "@deck.gl/core";
import type { OrthographicViewState, PickingInfo } from "@deck.gl/core";
import Flatbush from "flatbush";
import type { LayerName } from "../model";
import type { LodLevel, PreparedMap, RenderFeature } from "../render/types";
import { buildLayers } from "../render/layers";

const props = defineProps<{
  prepared?: PreparedMap;
  visibility: Record<LayerName, boolean>;
  selectedKey: string;
  resetRevision: number;
}>();
const emit = defineEmits<{ candidates: [keys: string[]] }>();
const host = ref<HTMLDivElement>();
let deck: Deck<OrthographicView> | undefined;
let currentView: OrthographicViewState = { target: [0, 0, 0], zoom: 0, minZoom: -8, maxZoom: 5 };
let dragging = false;
let spatialIndex: Flatbush | undefined;
let updateFrame = 0;

const lod = computed<LodLevel>(() => {
  const zoom = typeof currentView.zoom === "number" ? currentView.zoom : 0;
  if (zoom < 0) return "coarse";
  if (zoom < 2) return "medium";
  return "fine";
});

function visibleLabelKeys(): Set<string> | undefined {
  if (!spatialIndex || !host.value || !props.prepared) return undefined;
  const target = currentView.target ?? [0, 0, 0];
  const zoom = typeof currentView.zoom === "number" ? currentView.zoom : 0;
  const scale = 2 ** zoom;
  const halfWidth = host.value.clientWidth * 0.55 / scale;
  const halfHeight = host.value.clientHeight * 0.55 / scale;
  const matches = spatialIndex.search(
    target[0] - halfWidth, target[1] - halfHeight,
    target[0] + halfWidth, target[1] + halfHeight,
  );
  return new Set(matches.map((index) => props.prepared!.spatialFeatureKeys[index]));
}

function layers() {
  if (!props.prepared) return [];
  const zoom = typeof currentView.zoom === "number" ? currentView.zoom : 0;
  return buildLayers({
    prepared: props.prepared,
    visibility: props.visibility,
    selectedKey: props.selectedKey,
    lod: lod.value,
    visibleLabelKeys: visibleLabelKeys(),
    minimumLabelPriority: zoom < 0 ? 70 : zoom < 2 ? 55 : 0,
  });
}

function scheduleApply(): void {
  if (updateFrame) return;
  updateFrame = requestAnimationFrame(() => {
    updateFrame = 0;
    apply();
  });
}

function fitState(bounds = props.prepared?.bounds): OrthographicViewState {
  if (!bounds || !host.value) return { target: [0, 0, 0], zoom: 0, minZoom: -8, maxZoom: 5 };
  const width = Math.max(host.value.clientWidth, 320);
  const height = Math.max(host.value.clientHeight, 240);
  const spanX = Math.max(bounds.maxX - bounds.minX, 1);
  const spanY = Math.max(bounds.maxY - bounds.minY, 1);
  const scale = Math.max(1e-6, Math.min(width * 0.82 / spanX, height * 0.82 / spanY));
  return {
    target: [(bounds.minX + bounds.maxX) * 0.5, (bounds.minY + bounds.maxY) * 0.5, 0],
    zoom: Math.min(5, Math.log2(scale)), minZoom: -8, maxZoom: 5,
  };
}

function apply(reset = false): void {
  if (!deck) return;
  if (reset) currentView = fitState();
  if (host.value) host.value.dataset.viewZoom = String(currentView.zoom);
  deck.setProps({ viewState: currentView, layers: layers() });
  if (host.value) host.value.dataset.viewportZoom = String(deck.getViewports()[0]?.zoom ?? "");
}

function pickedFeature(info: PickingInfo): RenderFeature | undefined {
  const value = info.object as RenderFeature | undefined;
  return value?.key ? value : undefined;
}

onMounted(() => {
  if (!host.value) return;
  currentView = fitState();
  deck = new Deck<OrthographicView>({
    parent: host.value,
    width: "100%", height: "100%", views: new OrthographicView({ id: "map", flipY: false }),
    controller: true, viewState: currentView, layers: layers(), pickingRadius: 6,
    getCursor: ({ isDragging, isHovering }) => isDragging ? "grabbing" : isHovering ? "pointer" : "grab",
    onInteractionStateChange: (state) => { dragging = state.isDragging ?? false; },
    onViewStateChange: ({ viewState }) => {
      currentView = viewState as OrthographicViewState;
      if (host.value) host.value.dataset.viewZoom = String(currentView.zoom);
      scheduleApply();
    },
    onClick: (info) => {
      if (dragging || !deck) return;
      const picked = deck.pickMultipleObjects({ x: info.x, y: info.y, radius: 6, depth: 8 });
      const keys: string[] = [];
      for (const item of picked) {
        const feature = pickedFeature(item);
        if (feature && !keys.includes(feature.key)) keys.push(feature.key);
      }
      emit("candidates", keys);
    },
  });
});

watch(() => props.prepared, (prepared) => {
  spatialIndex = prepared?.spatialIndexData ? Flatbush.from(prepared.spatialIndexData) : undefined;
  apply(true);
});
watch(() => [props.visibility, props.selectedKey] as const, () => apply(), { deep: true });
watch(() => props.resetRevision, () => apply(true));
watch(() => props.selectedKey, (key) => {
  const feature = props.prepared?.features.find((item) => item.key === key);
  if (!feature?.bounds || !deck) return;
  currentView = fitState(feature.bounds);
  deck.setProps({ viewState: currentView, layers: layers() });
});

onBeforeUnmount(() => {
  if (updateFrame) cancelAnimationFrame(updateFrame);
  deck?.finalize();
  deck = undefined;
});
</script>

<template>
  <div ref="host" class="map-canvas" aria-label="Canonical 地图 WebGL 画布"></div>
</template>
