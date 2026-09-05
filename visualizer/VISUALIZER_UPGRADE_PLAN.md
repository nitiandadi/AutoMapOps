# Canonical 曲线与 WebGL 可视化升级计划

> 状态：已进入实施，进度与验证记录见 `VISUALIZER_EXECUTION_PLAN.md`  
> 目标：解决文本避让、对象选择、代码可维护性、大数据量和真实曲线渲染问题。

## 1. 总体方案

- 将现有原生 TypeScript + SVG 界面重构为 Vue 3 + TypeScript，并使用 deck.gl `OrthographicView` 作为二维 WebGL 渲染核心。
- deck.gl 直接使用本地 ENU 笛卡尔坐标，支持二进制缓冲、GPU 拾取和大规模 PathLayer，目标容量约为 10 万对象、100 万最终渲染顶点。
- 不选 Mapbox：当前不需要在线底图、经纬度投影、Token 和计费体系。未来需要真实底图时预留 MapLibre 适配层。
- 不直接选 Three.js：它更适合未来三维场景，但二维地图需要的标签、图层、拾取和 LOD 需要自行搭建。
- Canonical JSON 从数据层正式表达真实曲线；渲染层直接接收曲线结构并进行屏幕误差驱动的自适应细分，不把“平滑折线”作为曲线替代品。
- 本阶段不接入 OpenDRIVE，定位为静态二维地图查看器与调试器，不做地图编辑。

参考资料：

- [deck.gl OrthographicView](https://deck.gl/docs/api-reference/core/orthographic-view)
- [deck.gl 坐标系统](https://deck.gl/docs/developer-guide/coordinate-systems)
- [deck.gl 性能建议](https://deck.gl/docs/developer-guide/performance)

## 2. Canonical JSON 1.1 曲线协议

### 2.1 支持字段

以下三个字段支持点列或真实曲线：

- `Road.referenceLine`
- `Lane.centerline`
- `LaneBoundary.geometry`

`OperationalArea.outline`、`RestrictedArea.outline` 等区域轮廓继续使用闭合点列。

```ts
type PathGeometry3d =
  | Point3d[]
  | {
      type: "composite_curve";
      segments: CurveSegment3d[];
    };
```

### 2.2 曲线段格式

直线：

```json
{
  "type": "line",
  "start": [0, 0, 0],
  "headingRad": 0,
  "lengthM": 20,
  "endZM": 0
}
```

圆弧：

```json
{
  "type": "circular_arc",
  "start": [20, 0, 0],
  "headingRad": 0,
  "lengthM": 10,
  "endZM": 0.2,
  "curvaturePerM": 0.05
}
```

Clothoid 缓和曲线：

```json
{
  "type": "clothoid",
  "start": [29.59, 2.45, 0.2],
  "headingRad": 0.5,
  "lengthM": 15,
  "endZM": 0.5,
  "startCurvaturePerM": 0.05,
  "endCurvaturePerM": 0
}
```

### 2.3 统一语义

- 第一版曲线仅包含 `line`、`circular_arc`、`clothoid`。
- `lengthM` 是 XY 平面弧长。
- Z 从 `start[2]` 到 `endZM` 按弧长线性插值。
- `headingRad` 以 +X/东向为 0，逆时针为正，单位为弧度。
- 曲率单位为 `1/m`，正值左转、负值右转。
- 段顺序由数组顺序决定。
- 每段保留显式起点和起始航向，使不连续数据可以被原样读取，并由校验器报告。
- Schema 1.0 只允许旧点列；Schema 1.1 同时允许点列和曲线对象。
- 读取器支持 1.0 和 1.1，并拒绝未知版本。
- 写入时保留实际表示；要求输出 1.0 但数据包含曲线时明确报错，不静默离散化或升级版本。
- 默认新建地图版本及 `canonical_json_schema_version()` 更新为 `1.1`，已有 1.0 文件保持不变。
- 增加正式 JSON Schema，并据此生成前端 wire types；C++ 解析器继续使用强类型手工映射。

## 3. C++ 几何接口

新增以下类型：

- `LineCurveSegment3d`
- `CircularArcSegment3d`
- `ClothoidSegment3d`
- `CurveSegment3d = std::variant<...>`
- `CompositeCurve3d`
- `PathGeometry3d = Polyline3d | CompositeCurve3d`
- `CurveState3d { position, heading_rad, curvature_per_m }`

新增以下通用函数：

- `evaluate_path_geometry(path, s)`
- `path_start_state(path)`
- `path_end_state(path)`
- `path_planar_length(path)`
- `path_bounding_box(path)`
- `tessellate_path_geometry(path, options)`

`Road`、`Lane`、`LaneBoundary` 改用 `PathGeometry3d`；面积轮廓继续使用 `Polyline3d`。为旧点列提供初始化构造，尽量保持现有测试和示例的可读性。

Clothoid 使用线性曲率：

```text
k(s) = k0 + (k1 - k0) × s / L
```

航向解析计算，位置采用标准库实现的确定性自适应数值积分。C++ 与 TypeScript 使用共享黄金采样数据验证一致性。

## 4. I/O 与地图校验

- 所有直接依赖 `vector<Point3d>` 的长度、首尾点、包围盒、连接关系和可达性逻辑迁移到统一几何 API。
- 校验非有限数值、空曲线、非正长度、圆弧零曲率及坐标范围。
- 相邻曲线段连续性标准：
  - 端点三维距离大于 `1e-3 m`：G0 错误。
  - 航向差大于 `1e-4 rad`：G1 错误。
  - 曲率差大于 `1e-6 1/m`：G2 警告。
- 自相交等必须依赖折线的算法，统一使用最大 `0.01 m` 弦误差的确定性细分结果。
- 连接校验直接使用曲线精确端点和切向，不再依赖采样数组首尾点。
- CLI inspect 增加点列点数、各类曲线段数量、总弧长和包围盒统计。
- 文档说明 1.0/1.1 兼容性、坐标约定、曲线参数和后续格式转换边界。

## 5. Vue/WebGL 可视化界面

### 5.1 前端结构

- Vue 单文件组件负责工具栏、图层面板、对象列表、候选选择框和属性检查器。
- Pinia 只保存 UI 状态，完整地图对象不做深层响应式代理。
- deck.gl 使用 `OrthographicView` 和 `CARTESIAN` 坐标直接渲染 ENU。
- 渲染器保持独立服务，不为每个地图对象创建 Vue 组件。

### 5.2 渲染层直接支持曲线

实现 `CanonicalCurveLayer`，其公开输入直接为 `PathGeometry3d`：

- 点列按原折线渲染。
- line、arc、clothoid 在图层内部按当前米/像素比例自适应细分。
- 最大屏幕偏差目标为 `0.5 px`。
- 按缩放等级缓存粗细 LOD。
- 输出 deck.gl 二进制顶点缓冲并交给 PathLayer，避免百万级 JavaScript 对象开销。
- 内部细分只服务于 GPU 绘制，不覆盖、回写或降级 Canonical 曲线数据。

### 5.3 大数据加载

- Worker 完成 JSON 解析、Schema 校验、对象索引、包围盒、曲线求值、LOD 和二进制缓冲构建。
- 使用 Transferable ArrayBuffer 将结果传回主线程。
- 渲染坐标以地图中心平移后转为 Float32，属性检查器仍保留原始双精度 ENU 数值。
- 加载新文件时取消旧任务并终止旧 Worker，避免过期结果覆盖当前地图。
- 不再使用 `Math.min(...points)` 等存在大数组参数上限的实现。

## 6. 文本避让

- 路径标签锚点使用真实弧长中点。
- 区域标签使用内部代表点，点对象使用自身位置。
- 标签优先级固定为：当前选择对象 > 站点/路口 > 道路 > 区域 > 车道。
- 先按视口、缩放和空间索引筛选候选，再用 TextLayer 与 CollisionFilterExtension 实时避让。
- 当前选中对象的标签单独渲染并强制可见。
- 对密集区域设置按缩放等级生效的标签密度限制，避免把全部标签提交给 GPU。

参考资料：

- [deck.gl TextLayer](https://deck.gl/docs/api-reference/layers/text-layer)
- [deck.gl CollisionFilterExtension](https://deck.gl/docs/api-reference/extensions/collision-filter-extension)

## 7. 对象选择与属性检查

- 所有对象使用 `kind + collectionIndex + id` 作为内部稳定键，正确处理重复业务 ID。
- 地图拾取使用 GPU picking、6 px 容差和多对象深度拾取。
- 单候选直接选中；多个重叠候选显示候选列表。
- 对象选择器改为可搜索、分类、分页列表，支持按 ID、名称、类型查找。
- Junction、VehicleProfile 等无直接几何对象也可通过对象列表选择。
- 地图拾取、对象列表、属性检查器和高亮共享同一选中状态。
- 选择对象后自动定位，并使用独立图层显示高亮。
- 图层开关在重新加载数据后保持，不再被初始化流程重置。
- 保留内置 1.0 示例，并增加最小 1.1 line/arc/clothoid 连续曲线示例。

参考资料：[deck.gl 交互与拾取](https://deck.gl/docs/developer-guide/interactivity)

## 8. 测试与验收

### 8.1 几何与协议测试

- C++ 单元测试覆盖直线、正负圆弧、clothoid、混合曲线、Z 插值、端点、航向、曲率和包围盒。
- C++/TypeScript 对同一黄金数据采样，位置差不超过 `1e-6 m`。
- I/O 测试覆盖 1.0 点列读取、1.1 点列与曲线往返、未知版本、未知段类型、错误参数及曲线写入 1.0 的失败行为。
- 校验测试覆盖非有限值、负长度、零曲率圆弧、G0/G1 错误、G2 警告，以及曲线道路的连接与可达性。

### 8.2 前端功能测试

- 单测覆盖 Schema 错误路径、Worker 取消、曲线细分、LOD 缓存、标签锚点、重复 ID 稳定键和搜索索引。
- 浏览器测试覆盖图层状态保持、对象搜索和定位、宽容拾取、重叠对象候选框、检查器同步、密集标签避让，以及 1.0/1.1 示例加载。

### 8.3 性能目标

性能测试运行时生成约 10 万对象、100 万最终顶点，不提交巨型固定文件。在开发机 Chrome 1080p 环境记录硬件和测试结果，目标为：

- 解析和几何构建期间界面不冻结。
- 首个粗粒度画面不超过 5 秒。
- 常规平移缩放不低于 30 FPS。
- 加载完成后，搜索、选择和检查器反馈 P95 不超过 100 ms。

## 9. 本阶段边界

本次不实现：

- OpenDRIVE 导入。
- Mapbox/MapLibre 底图。
- 三维视图。
- 地图几何编辑。
- Bézier 曲线。
- 区域轮廓曲线化。

后续若需要真实地理底图，在渲染服务边界增加 MapLibre 适配，不修改 Canonical JSON。该查看器仍属于教学和调试工具，不宣称可以直接用于量产车辆。

## 10. 实施约束

- 保留工作区现有未提交改动，不重置、不覆盖无关文件。
- 测试和性能基准随实现提供。
- 按当前项目约定，新增地图数据和学习示例默认交付给用户自行验证；只有用户明确要求或反馈具体问题时，再执行对应查看器、解析器或测试命令。
