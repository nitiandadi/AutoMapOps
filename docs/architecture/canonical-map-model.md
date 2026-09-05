# Canonical Map Model 架构

## 目的

本文冻结 M2 的代码骨架，说明 Canonical 地图模型的分层、所有权、引用方式和模块边界。该模型是 AutoMapOps 的教学型内部格式，不是 OpenDRIVE、Lanelet2 或其他行业标准。

## 分层

```text
基础值类型
  Point3d / Polyline3d / BoundingBox3d / GeodeticPoint
        ↓
地图元数据
  MapHeader / CoordinateReference
        ↓
地图业务对象
  Road / Lane / LaneBoundary / Junction / LaneConnection
  OperationalArea / Station / RestrictedArea / VehicleProfile
        ↓
聚合根
  MapData
```

依赖只能从下层指向上层使用的基础类型，基础几何不得依赖地图业务对象。

## 坐标和单位

- `Point3d` 表示地图内部的局部 ENU 坐标，X 向东、Y 向北、Z 向上，单位为米；
- `GeodeticPoint` 只表示 WGS84 原点，经纬度单位为度，高程单位为米；
- 两种点类型不能隐式转换，避免将经纬度误当作局部米制坐标；
- 距离、宽度和高度使用米，速度使用米每秒，角度使用弧度；
- 普通浮点比较默认容差为 `1e-6`，坐标点重合默认容差为 `1e-3 m`；
- 车道连接距离、航向差等业务阈值属于 `map_validation`，不写入基础几何类型。

## 所有权与引用

`MapData` 按值持有全部地图对象。对象之间只保存稳定字符串 ID，不保存指针或引用：

```text
MapData owns Lane
Lane.left_boundary_id ──► LaneBoundary.id
Lane.successor_ids    ──► Lane.id
Junction.connection_ids ──► LaneConnection.id
```

这样可以安全复制地图快照、直接映射 JSON，并为后续不可变 `MapVersion` 提供稳定输入。

## 容器选择

M2 使用 `std::vector` 保存对象，以保留 Canonical JSON 的稳定顺序，并允许暂时承载重复 ID、悬空引用等待质检的数据。`MapData::find_*` 提供便捷查找，但不代替 M3 的唯一性和引用完整性检查。

第一版不维护第二套长期存在的拓扑图或对象索引。路径规划和运行时编译阶段可以从权威 ID 引用构建专用邻接表与索引。

## 模块职责

| 模块 | 职责 |
|---|---|
| `map_core` | 数据表达、基础几何计算和无业务阈值的值类型 |
| `map_io` | Canonical JSON 与 C++ 模型之间转换 |
| `map_validation` | ID、引用、几何、拓扑和可达性规则 |
| `map_version` | 发布资格、版本元数据和不可变快照 |

例如，折线长度计算属于 `map_core`；判断 Lane 是否过短、前后继是否互反属于 `map_validation`。

## M2 实施顺序

1. 完成基础几何与测试；
2. 完成 `MapHeader`、Road、Lane 和 LaneBoundary；
3. 完成 Junction、场景对象、车辆约束和 `MapData`；
4. 设计物流园拓扑并生成 V0 Canonical JSON。
# 路径几何扩展

从 Canonical Schema 1.1 开始，道路参考线、车道中心线和车道边界统一使用 `PathGeometry3d`。该值类型持有旧 `Polyline3d` 或 `CompositeCurve3d`，因此生产数据可以保留 line、circular arc 和 clothoid 的真实参数，而需要折线的质检或渲染算法通过统一细分 API 获取临时视图。

区域轮廓继续使用 `Polyline3d`。这可以维持首版区域闭合、面积和点内判断逻辑，不把路径曲线改造扩展为通用曲面建模。

反向车道不反转存储数据。算法使用 `PathTraversal::reverse` 获取实际行驶方向状态：位置沿路径反向求值、航向增加 π、曲率符号反转。

