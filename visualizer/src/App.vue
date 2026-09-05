<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted } from "vue";
import { storeToRefs } from "pinia";
import CandidatePicker from "./components/CandidatePicker.vue";
import InspectorPanel from "./components/InspectorPanel.vue";
import LayerPanel from "./components/LayerPanel.vue";
import MapCanvas from "./components/MapCanvas.vue";
import ObjectBrowser from "./components/ObjectBrowser.vue";
import type { LayerName } from "./model";
import { sampleMap } from "./sample-map";
import { useMapStore } from "./stores/map-store";
import { MapWorkerClient } from "./workers/map-worker-client";

const store = useMapStore();
const {
  prepared, source, loading, error, selectedKey, selected, candidates,
  searchQuery, searchResults, resetViewRevision, layerVisibility,
  queryMode, selectionAnchor, focusRevision,
} = storeToRefs(store);
const worker = new MapWorkerClient();

const status = computed(() => {
  if (loading.value) return "正在解析、校验并构建 WebGL 几何…";
  if (error.value) return `读取失败：${error.value}`;
  if (!prepared.value) return "尚未加载地图";
  const stats = prepared.value.stats;
  return `${source.value} · ${stats.objectCount} 个对象 · ${stats.pathCount} 条路径 · ${stats.renderedVertexCount} 个精细 LOD 顶点`;
});

async function loadDemo(): Promise<void> {
  store.setLoading();
  try {
    store.setPrepared(await worker.loadMap(sampleMap, "OpenDRIVE 1.8 物流园转换示例"), "OpenDRIVE 1.8 物流园转换示例");
  } catch (reason) {
    store.setError(reason instanceof Error ? reason.message : "无法加载内置示例");
  }
}

async function openFile(event: Event): Promise<void> {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  input.value = "";
  if (!file) return;
  store.setLoading();
  try {
    store.setPrepared(await worker.loadText(await file.text(), file.name), file.name);
  } catch (reason) {
    store.setError(reason instanceof Error ? reason.message : "无效 JSON");
  }
}

function toggleLayer(layer: LayerName, visible: boolean): void {
  store.toggleLayer(layer, visible);
}

onMounted(loadDemo);
onBeforeUnmount(() => worker.dispose());
</script>

<template>
  <header class="topbar">
    <div>
      <p class="eyebrow">CANONICAL MAP DEBUGGER · WEBGL</p>
      <h1>AutoMapOps Visualizer</h1>
    </div>
    <div class="toolbar">
      <div class="query-modes" role="group" aria-label="查询模式">
        <button :aria-pressed="queryMode === 'table'" @click="store.setQueryMode('table')">属性表</button>
        <button :aria-pressed="queryMode === 'json'" @click="store.setQueryMode('json')">原始 JSON</button>
      </div>
      <label class="button primary" for="map-file">打开 JSON</label>
      <input id="map-file" type="file" accept="application/json,.json" hidden @change="openFile" />
      <button class="button" type="button" @click="loadDemo">加载物流园示例</button>
      <button class="button" type="button" @click="store.resetView">重置视图</button>
    </div>
  </header>

  <main class="workspace" :class="{ 'with-inspector': queryMode === 'json' }">
    <aside class="panel layers-panel">
      <section>
        <p class="section-label">当前地图</p>
        <h2>{{ prepared?.map.header.name || "—" }}</h2>
        <p class="muted">{{ prepared?.map.header.mapId || "—" }} · Schema {{ prepared?.map.header.schemaVersion || "—" }}</p>
        <p v-if="prepared" class="muted">渲染原点偏移：{{ prepared.origin[0].toFixed(3) }}, {{ prepared.origin[1].toFixed(3) }} m</p>
      </section>
      <LayerPanel :visibility="layerVisibility" @toggle="toggleLayer" />
      <ObjectBrowser
        v-model:query="searchQuery"
        :results="searchResults"
        :selected-key="selectedKey"
        @select="store.select"
      />
      <section class="legend">
        <p class="section-label">图例</p>
        <span><i class="swatch lane"></i>Lane 中心线</span>
        <span><i class="swatch boundary"></i>LaneBoundary</span>
        <span><i class="swatch road"></i>Road 参考线</span>
        <span><i class="swatch warehouse"></i>仓库</span>
        <span><i class="swatch loading"></i>装卸区</span>
        <span><i class="swatch parking"></i>停车区</span>
        <span><i class="swatch charging"></i>充电区</span>
        <span><i class="swatch restricted"></i>限制区域</span>
        <span><i class="swatch station"></i>Station</span>
      </section>
    </aside>

    <section class="map-shell">
      <div class="status" :class="{ error: Boolean(error) }">{{ status }}</div>
      <MapCanvas
        :prepared="prepared"
        :visibility="layerVisibility"
        :selected-key="selectedKey"
        :reset-revision="resetViewRevision"
        :focus-revision="focusRevision"
        :query-mode="queryMode"
        :selection-anchor="selectionAnchor"
        @candidates="store.showCandidates"
        @select="store.select"
        @close="store.clearSelection"
        @json="store.setQueryMode('json')"
      />
      <CandidatePicker
        :candidates="candidates"
        @select="store.selectCandidate"
        @close="store.showCandidates([])"
      />
      <div v-if="loading" class="loading-mask"><span></span><strong>正在处理地图数据</strong></div>
    </section>

    <InspectorPanel v-if="queryMode === 'json'" :selected="selected" @close="store.setQueryMode('table')" />
  </main>
</template>
