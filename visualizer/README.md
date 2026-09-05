# AutoMapOps Visualizer

基于 Vue 3、TypeScript 和 deck.gl 的 Canonical Map 二维 WebGL 调试器。它直接读取本地 ENU 坐标，不依赖在线底图或 Mapbox Token。

## 当前能力

- 读取 Canonical JSON 1.0 点列和 1.1 真实曲线；
- 直接显示 `line`、`circular_arc`、`clothoid` 组合曲线；
- 显示 Road、Lane、LaneBoundary、区域、Station、Junction 和 Lane 拓扑；
- Worker 中解析、Schema 校验、构建三档 LOD 和二进制路径缓冲；
- GPU 文本碰撞过滤和按对象类型划分的标签优先级；
- 6 px 容差拾取、重叠候选选择、对象搜索和属性检查；
- 图层开关、平移、缩放、对象定位和视图重置；
- 以地图中心平移渲染坐标，属性检查器保留原始 ENU 数值。

## 运行

```powershell
cd visualizer
npm install
npm run dev
```

静态检查与构建：

```powershell
npm run generate:types
npm run typecheck
npm run build
```

测试和性能基准：

```powershell
npm run test
npm run test:e2e
npm run benchmark
```

## 数据协议

正式协议位于 `../schemas/canonical-map-1.1.schema.json`。三个路径字段接受点列或曲线对象：

- `Road.referenceLine`
- `Lane.centerline`
- `LaneBoundary.geometry`

```json
{
  "type": "composite_curve",
  "segments": [
    {
      "type": "circular_arc",
      "start": [20.0, 0.0, 0.5],
      "headingRad": 0.0,
      "lengthM": 10.0,
      "endZM": 1.0,
      "curvaturePerM": 0.05
    }
  ]
}
```

内置示例为 `maps/opendrive/logistics_park_v0.xodr` 的 Canonical 1.1 转换结果：

- 输出：`maps/drafts/logistics_park_from_opendrive_v1_1.json`
- 转换器：`examples/opendrive/opendrive_to_canonical_1_1.py`
- 道路参考线、车道、边界和连接拓扑来自 OpenDRIVE；
- 仓库、装卸区、停车区、充电区、站点和限制区使用该 XODR 的原始 Canonical V0 来源补回精确业务多边形。

重新生成示例：

```powershell
python ..\examples\opendrive\opendrive_to_canonical_1_1.py `
  ..\maps\opendrive\logistics_park_v0.xodr `
  ..\maps\drafts\logistics_park_v0.json `
  ..\maps\drafts\logistics_park_from_opendrive_v1_1.json
```

仓库中的 `maps/drafts/logistics_park_v0.json` 仍保持 Schema 1.0，也可直接打开。

## 对象查询

工具栏提供「属性表 / 原始 JSON」两种模式，并在本机记住选择。默认属性表模式使用地图浮动卡片，显示中文字段、单位、关联对象链接和可展开的曲线段参数。点击地图保持视角；从列表或关联链接选择时定位对象。无几何对象显示在画布右上角。关闭卡片或点击地图空白处可清除选中。

原始 JSON 模式展开右侧面板，提供完整字段和复制按钮；切换模式保留选中对象。属性表模式收起右侧面板以扩大地图空间。

## 渲染实现

`CanonicalCurveLayer` 的输入仍是原始 `PathGeometry3d`。Worker 会为 GPU 生成 coarse、medium、fine 三档自适应细分缓存；该缓存不替代或修改 Canonical 曲线数据。PathLayer 使用 typed array 二进制属性，避免为每个渲染顶点创建 JavaScript 对象。

Canonical 中的原始 Z 高程完整保留在地图模型、对象检查器和精确计算中。当前二维 WebGL 画布使用压平到 Z=0 的渲染副本，避免正高程被正交相机近裁剪面截断。

## 定位与限制

本工具是二维只读教学和调试器。当前页面读取的是转换后的 Canonical JSON；OpenDRIVE 到 Canonical 的导入通过仓库脚本离线完成。当前不提供在线底图、三维显示和地图编辑功能，也不应作为量产车辆地图组件直接使用。
