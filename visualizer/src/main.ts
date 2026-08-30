import "./styles.css";
import type { LayerName, SelectedObject } from "./renderer";
import type { MapData } from "./model";
import { mapId, mapName } from "./model";
import { MapRenderer } from "./renderer";
import { sampleMap } from "./sample-map";

const app = document.querySelector<HTMLDivElement>("#app");
if (!app) throw new Error("找不到应用根节点 #app。");

app.innerHTML = `
  <header class="topbar">
    <div>
      <p class="eyebrow">CANONICAL MAP DEBUGGER</p>
      <h1>AutoMapOps Visualizer</h1>
    </div>
    <div class="toolbar">
      <label class="button primary" for="map-file">打开 JSON</label>
      <input id="map-file" type="file" accept="application/json,.json" hidden />
      <button id="load-demo" class="button" type="button">加载示例</button>
      <button id="reset-view" class="button" type="button">重置视图</button>
    </div>
  </header>
  <main class="workspace">
    <aside class="panel layers-panel">
      <section>
        <p class="section-label">当前地图</p>
        <h2 id="map-name">—</h2>
        <p id="map-id" class="muted">—</p>
      </section>
      <section>
        <p class="section-label">图层</p>
        <div id="layer-list" class="layer-list"></div>
      </section>
      <section class="legend">
        <p class="section-label">图例</p>
        <span><i class="swatch lane"></i>Lane 中心线</span>
        <span><i class="swatch boundary"></i>LaneBoundary</span>
        <span><i class="swatch road"></i>Road 参考线</span>
        <span><i class="swatch area"></i>业务区域</span>
        <span><i class="swatch restricted"></i>限制区域</span>
        <span><i class="swatch station"></i>Station</span>
      </section>
    </aside>
    <section class="map-shell">
      <div id="status" class="status">滚轮缩放 · 按住左键拖动画布 · 点击对象查看属性</div>
      <svg id="map-canvas" role="img" aria-label="Canonical 地图画布"></svg>
    </section>
    <aside class="panel inspector-panel">
      <p class="section-label">对象检查器</p>
      <h2 id="object-title">尚未选择对象</h2>
      <p class="muted">点击地图中的线、区域或点，查看 Canonical 字段。</p>
      <pre id="object-json">{}</pre>
    </aside>
  </main>
`;

const svg = document.querySelector<SVGSVGElement>("#map-canvas")!;
const mapNameElement = document.querySelector<HTMLElement>("#map-name")!;
const mapIdElement = document.querySelector<HTMLElement>("#map-id")!;
const objectTitle = document.querySelector<HTMLElement>("#object-title")!;
const objectJson = document.querySelector<HTMLElement>("#object-json")!;
const status = document.querySelector<HTMLElement>("#status")!;

const showSelection = ({ kind, value }: SelectedObject): void => {
  const id = "id" in value && typeof value.id === "string" ? value.id : "未命名对象";
  objectTitle.textContent = `${kind} · ${id}`;
  objectJson.textContent = JSON.stringify(value, null, 2);
};

const renderer = new MapRenderer(svg, showSelection);
let currentMap: MapData = sampleMap;

const layerLabels: Record<LayerName, string> = {
  areas: "业务区域",
  restricted: "限制区域",
  roads: "Road 参考线",
  boundaries: "LaneBoundary",
  lanes: "Lane 中心线",
  topology: "拓扑箭头",
  junctions: "路口连接",
  stations: "Station",
  labels: "对象标签",
};

const layerList = document.querySelector<HTMLDivElement>("#layer-list")!;
Object.entries(layerLabels).forEach(([layer, label]) => {
  const item = document.createElement("label");
  item.className = "layer-toggle";
  item.innerHTML = `<input type="checkbox" checked /> <span>${label}</span>`;
  item.querySelector("input")!.addEventListener("change", (event) => {
    renderer.setLayerVisible(layer as LayerName, (event.target as HTMLInputElement).checked);
  });
  layerList.append(item);
});

function render(map: MapData, source: string): void {
  currentMap = map;
  renderer.render(currentMap);
  mapNameElement.textContent = mapName(map);
  mapIdElement.textContent = mapId(map);
  objectTitle.textContent = "尚未选择对象";
  objectJson.textContent = "{}";
  status.textContent = `${source} · ${map.lanes?.length || 0} 条 Lane · ${map.stations?.length || 0} 个 Station`;
}

document.querySelector("#load-demo")!.addEventListener("click", () => render(sampleMap, "内置示例"));
document.querySelector("#reset-view")!.addEventListener("click", () => renderer.resetView());
document.querySelector<HTMLInputElement>("#map-file")!.addEventListener("change", async (event) => {
  const file = (event.target as HTMLInputElement).files?.[0];
  if (!file) return;
  try {
    const parsed = JSON.parse(await file.text()) as MapData;
    render(parsed, file.name);
  } catch (error) {
    status.textContent = `读取失败：${error instanceof Error ? error.message : "无效 JSON"}`;
  }
});

render(currentMap, "内置示例");
