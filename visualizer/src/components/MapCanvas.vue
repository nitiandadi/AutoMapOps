<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, watch } from "vue";
import { Deck, OrthographicView } from "@deck.gl/core";
import type { OrthographicViewState, PickingInfo } from "@deck.gl/core";
import Flatbush from "flatbush";
import type { LayerName, Point3d } from "../model";
import PropertyPopup from "./PropertyPopup.vue";
import type { LodLevel, PreparedMap, RenderFeature } from "../render/types";
import { buildLayers } from "../render/layers";

const props = defineProps<{
  prepared?: PreparedMap;
  visibility: Record<LayerName, boolean>;
  selectedKey: string;
  resetRevision: number;
  focusRevision: number;
  queryMode: "table" | "json";
  selectionAnchor?: Point3d;
}>();
const emit = defineEmits<{ candidates: [keys: string[], anchor?: Point3d]; select: [key: string]; close: []; json: [] }>();
const host = ref<HTMLDivElement>();
const popupStyle = ref({ left: "16px", top: "64px" });
const selected = computed(() => props.prepared?.features.find(f => f.key === props.selectedKey));
let resizeObserver: ResizeObserver | undefined;

function positionPopup(): void {
  if (!host.value) return;
  const width = host.value.clientWidth;
  const height = host.value.clientHeight;
  const point = props.selectionAnchor ?? selected.value?.position;
  const target = currentView.target ?? [0, 0, 0];
  const scale = 2 ** (typeof currentView.zoom === "number" ? currentView.zoom : 0);
  const x = point ? width / 2 + (point[0] - target[0]) * scale : width;
  const y = point ? height / 2 - (point[1] - target[1]) * scale : 64;
  const popupWidth = Math.min(360, width - 24);
  const popupHeight = Math.min(480, height - 80);
  popupStyle.value = {
    left: `${Math.max(12, Math.min(x + 16, width - popupWidth - 12))}px`,
    top: `${Math.max(64, Math.min(y + 12, height - popupHeight - 12))}px`,
  };
}
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
  positionPopup();
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
      const coordinate = deck.getViewports()[0]?.unproject([info.x, info.y]);
      emit("candidates", keys, coordinate ? [coordinate[0], coordinate[1], 0] : undefined);
    },
  });
  resizeObserver = new ResizeObserver(() => { scheduleApply(); });
  resizeObserver.observe(host.value);
});

watch(() => props.prepared, (prepared) => {
  spatialIndex = prepared?.spatialIndexData ? Flatbush.from(prepared.spatialIndexData) : undefined;
  apply(true);
});
watch(() => [props.visibility, props.selectedKey] as const, () => apply(), { deep: true });
watch(() => props.resetRevision, () => apply(true));
watch(() => props.focusRevision, () => {
  const feature = selected.value;
  if (!feature?.bounds || !deck) return;
  currentView = fitState(feature.bounds);
  apply();
});
watch(() => [props.selectionAnchor, props.selectedKey, props.queryMode], positionPopup, { flush: "post" });

onBeforeUnmount(() => {
  if (updateFrame) cancelAnimationFrame(updateFrame);
  resizeObserver?.disconnect();
  deck?.finalize();
  deck = undefined;
});
</script>

<template>
  <div ref="host" class="map-canvas" aria-label="Canonical 地图 WebGL 画布"></div>
  <PropertyPopup v-if="queryMode === 'table' && selected && prepared"
    :style="popupStyle" :selected="selected" :features="prepared.features"
    @select="emit('select', $event)" @close="emit('close')" @json="emit('json')" />
</template>
