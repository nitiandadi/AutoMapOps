# Road 与 Lane 模型

## 作用

`Road` 组织同一条道路上的 Lane，并提供统一的参考线；`Lane` 表达车辆实际使用的车道几何、方向、状态、宽度、限速及拓扑引用。

本模型是 AutoMapOps 的 Canonical 内部模型。它为后续导出 OpenDRIVE 保留必要语义，但不直接复制 OpenDRIVE 的 XML 结构。

## Road

| C++ 字段 | JSON 字段 | 含义 |
|---|---|---|
| `id` | `id` | Road 稳定业务 ID |
| `name` | `name` | 显示名称 |
| `reference_line` | `referenceLine` | Road 参考线，点序决定正向 |
| `predecessor_ids` | `predecessorIds` | 普通道路级前驱引用 |
| `successor_ids` | `successorIds` | 普通道路级后继引用 |
| `lane_ids` | `laneIds` | 该 Road 持有的 Lane 引用 |

参考线使用局部 ENU 米制坐标。沿点序从第一个点走向最后一个点，称为 `along_reference_line`。

## Lane

| C++ 字段 | JSON 字段 | 含义 |
|---|---|---|
| `id` | `id` | Lane 稳定业务 ID |
| `road_id` | `roadId` | 所属 Road |
| `centerline` | `centerline` | Lane 中心线，局部 ENU，单位米 |
| `side` | `side` | 相对 Road 参考线位于 `left` 或 `right` |
| `order_from_reference` | `orderFromReference` | 从参考线向外计数，最内侧为 1 |
| `left_boundary_id` | `leftBoundaryId` | 沿 Lane 行驶方向观察的左边界 |
| `right_boundary_id` | `rightBoundaryId` | 沿 Lane 行驶方向观察的右边界 |
| `predecessor_ids` | `predecessorIds` | Lane 前驱引用 |
| `successor_ids` | `successorIds` | Lane 后继引用 |
| `direction` | `direction` | 行驶方向相对 Road 参考线的关系 |
| `status` | `status` | 首版为 `open` 或 `closed` |
| `width_m` | `widthM` | 首版恒定宽度，单位米 |
| `speed_limit_mps` | `speedLimitMps` | 限速，单位米每秒 |

`side` 和 `direction` 是两个独立概念：

```text
side      回答 Lane 几何位于参考线哪一侧
direction 回答车辆沿参考线正向还是反向行驶
```

在典型 RHT 双向道路中，可以表示为：

```text
左侧 Lane：side=left，direction=against_reference_line
参考线：    起点 ─────────────────────────► 终点
右侧 Lane：side=right，direction=along_reference_line
```

这种显式表达避免将 OpenDRIVE 的有符号 Lane ID 当作内部业务 ID。导出器以后可以根据 `side` 和 `order_from_reference` 生成 Lane ID：左侧为正数，右侧为负数。

## 拓扑与所有权

`MapData` 按值持有 Road 和 Lane；对象之间只保存稳定 ID：

```text
Road.lane_ids ───────────────► Lane.id
Lane.road_id ────────────────► Road.id
Lane.predecessor_ids ────────► Lane.id
Lane.successor_ids ──────────► Lane.id
Lane.left/right_boundary_id ─► LaneBoundary.id
```

M2 允许模型暂时承载重复 ID 或悬空引用。M3 的 `map_validation` 负责检查 ID 唯一性、引用完整性、前后继互反以及连接几何连续性。

## 首版边界

- Road 参考线和 Lane 中心线暂时使用离散 `Polyline3d`；
- Lane 宽度暂时为常数，不表达分段多项式；
- Lane 状态只有开放和关闭；
- 路口分支关系由后续 `Junction` 与 `LaneConnection` 表达；
- 导出 OpenDRIVE 时，第一版可将参考线折线逐段输出为 `<geometry><line>`，以后再增加 line/arc/spiral 拟合。
