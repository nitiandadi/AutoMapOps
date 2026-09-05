import { computed, markRaw, ref } from "vue";
import { defineStore } from "pinia";
import type { LayerName, SelectedObject } from "../model";
import type { PreparedMap, RenderFeature } from "../render/types";

const defaultLayerVisibility: Record<LayerName, boolean> = {
  areas: true, restricted: true, roads: true, boundaries: true, lanes: true,
  topology: true, junctions: true, stations: true, labels: true,
};

export const useMapStore = defineStore("map", () => {
  const prepared = ref<PreparedMap>();
  const source = ref("内置示例");
  const loading = ref(false);
  const error = ref("");
  const selectedKey = ref("");
  const candidateKeys = ref<string[]>([]);
  const searchQuery = ref("");
  const searchPage = ref(0);
  const resetViewRevision = ref(0);
  const layerVisibility = ref<Record<LayerName, boolean>>({ ...defaultLayerVisibility });

  const selected = computed<SelectedObject | undefined>(() =>
    prepared.value?.features.find((feature) => feature.key === selectedKey.value));
  const candidates = computed<RenderFeature[]>(() => {
    const keys = new Set(candidateKeys.value);
    return prepared.value?.features.filter((feature) => keys.has(feature.key)) ?? [];
  });
  const searchResults = computed<RenderFeature[]>(() => {
    const query = searchQuery.value.trim().toLocaleLowerCase();
    const all = prepared.value?.features.filter((feature) => feature.kind !== "LaneTopology") ?? [];
    if (!query) return all;
    return all.filter((feature) => `${feature.id} ${feature.name} ${feature.kind}`.toLocaleLowerCase().includes(query));
  });

  function setPrepared(result: PreparedMap, nextSource: string): void {
    prepared.value = markRaw(result);
    source.value = nextSource;
    loading.value = false;
    error.value = "";
    selectedKey.value = "";
    candidateKeys.value = [];
    searchPage.value = 0;
    resetViewRevision.value += 1;
  }

  function setLoading(): void {
    loading.value = true;
    error.value = "";
    candidateKeys.value = [];
  }

  function setError(message: string): void {
    loading.value = false;
    error.value = message;
  }

  function select(key: string): void {
    selectedKey.value = key;
    candidateKeys.value = [];
  }

  function showCandidates(keys: string[]): void {
    const unique = [...new Set(keys)];
    if (unique.length === 1) select(unique[0]);
    else candidateKeys.value = unique;
  }

  function toggleLayer(layer: LayerName, visible: boolean): void {
    layerVisibility.value[layer] = visible;
  }

  function resetView(): void {
    resetViewRevision.value += 1;
  }

  return {
    prepared, source, loading, error, selectedKey, selected, candidateKeys, candidates,
    searchQuery, searchPage, searchResults, resetViewRevision, layerVisibility,
    setPrepared, setLoading, setError, select, showCandidates, toggleLayer, resetView,
  };
});
