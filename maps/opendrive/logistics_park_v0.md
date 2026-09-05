# 物流园 Canonical V0 到 OpenDRIVE 的转换说明

## 产物

- 输入：`maps/drafts/logistics_park_v0.json`
- 输出：`maps/opendrive/logistics_park_v0.xodr`
- 转换示例：`examples/opendrive/canonical_v0_to_opendrive.py`

运行方式：

```powershell
python examples/opendrive/canonical_v0_to_opendrive.py `
  maps/drafts/logistics_park_v0.json `
  maps/opendrive/logistics_park_v0.xodr
```

转换器仅使用 Python 标准库，针对当前物流园 V0 的固定拓扑，不是通用 Canonical/OpenDRIVE 转换器。

## 拓扑展开

Canonical V0 为满足 M2 的单路口教学范围，把骨架 J20 简化为 `lane_exit_inner` 的双前驱汇流，并把汇流几何并入 `road_main`、`road_detour` 末端。

OpenDRIVE 普通 `<road><link>` 只能保存一个 predecessor，无法让 `road_exit` 同时直接引用主路和绕行路。因此导出时恢复：

```text
Road 2 ─► Junction 20 ─► Road 201 ─┐
                                    ├─► Road 4
Road 3 ─► Junction 20 ─► Road 202 ─┘
```

- Road 201 是双车道直行连接道路；
- Road 202 是单车道左转汇流连接道路；
- Junction 20 和两条连接道路使用 `automap.exportRole=synthesized_merge_*` 标记；
- 该展开没有新增可选路线，只把 Canonical 的双前驱汇流转换成 OpenDRIVE 可表达的结构。

因此输出包含 9 条 Road、15 条 OpenDRIVE Lane 和 2 个 Junction。Lane 数增加来自被切分出来的路口内连接段，不代表 Canonical 新增了业务车道。

## 几何与车道映射

- Road 参考线恢复为骨架中的精确 `line` 和 `arc`；
- Canonical 固定宽度写入 `<lane><width>`；
- Canonical 限速写入 Road 级 `<type><speed>` 和 Lane 级 `<speed>`；
- Canonical 全局 Lane ID 写入 Lane 的 `automap.canonicalLaneId`；
- Canonical 相邻车道共享的虚线转换为 `roadMark type="broken" laneChange="both"`；
- `curb` 外边界在当前 OpenDRIVE Lane roadMark 中近似为不可跨越实线，不能无损保留独立边界对象结构。

输出沿用骨架的局部横轴墨卡托定义：

```text
+proj=tmerc +lat_0=30.5728 +lon_0=104.0668 +k=1
+x_0=0 +y_0=0 +ellps=WGS84 +units=m +no_defs
```

当前 V0 的局部 ENU 数值直接作为该小范围教学投影的平面坐标。真实地图导出时应执行显式 ENU、WGS84 和目标投影转换，不能把这种小范围近似当作量产测绘方法。

## 业务对象映射

| Canonical 对象 | OpenDRIVE 表达 | 完整语义保存位置 |
|---|---|---|
| 门岗 Station | `building/tollBooth` | `userData` |
| 仓库 OperationalArea | `building` | `userData` |
| 装卸区 | `parkingSpace/openSpace` | `userData` |
| A1 月台 Station | `parkingSpace/openSpace` | `userData` 中保存 `loading_bay` 和接入 Lane |
| 停车区、停车位 | `parkingSpace` | `userData` |
| 充电区 | 限制为 electric 的 `parkingSpace` | `userData` |
| 充电桩 Station | `obstacle/chargingStation` | `userData` 中保存接入 Lane |
| 窄通道 RestrictedArea | 可通行的 `roadSurface/other` | `userData` 中保存车辆白名单 |

这些映射以教学和可追溯为目标。OpenDRIVE 没有统一的物流月台任务、Station 接入 Lane 或 Canonical 车辆白名单模型，读取方若忽略 `userData`，只能得到降级后的设施几何。

## 限制

- 不是完整、通用的 Canonical/OpenDRIVE 双向转换；
- 未输出坡度、超高、信号灯、停止线和交通优先权；
- 没有把所有 Canonical Boundary 独立对象无损映射到 OpenDRIVE；
- OpenDRIVE 对象使用道路局部 `s/t` 包围盒，业务区域的精确原始 Polygon 仍以 Canonical JSON 为准；
- 输出是教学草稿，不能直接用于量产车辆或安全关键决策。

## 转回 Canonical 1.1 并可视化

可使用反向转换器把本文件的道路几何、车道宽度和 Junction 拓扑还原为支持曲线的 Canonical 1.1：

```powershell
python examples/opendrive/opendrive_to_canonical_1_1.py `
  maps/opendrive/logistics_park_v0.xodr `
  maps/drafts/logistics_park_v0.json `
  maps/drafts/logistics_park_from_opendrive_v1_1.json
```

输出包含 9 条道路、15 条车道、24 条边界、2 个 Junction 和 6 条 LaneConnection。OpenDRIVE `planView` 中的 `line`、`arc`、`spiral` 会保留为 Canonical 1.1 曲线段，不会在数据层离散成折线。

因为 XODR 业务对象只保存了道路局部 `s/t` 包围盒，转换器使用该文件可追溯的源数据 `maps/drafts/logistics_park_v0.json` 补回精确的园区多边形、站点和车辆限制语义；这部分不是从简化后的 XODR 对象轮廓反推得到的。
