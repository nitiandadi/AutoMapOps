# 地图质检规则

## 规则执行边界

`map_validation` 对已经读入 `MapData` 的工作草稿执行规则检查。规则通过 `ValidationContext` 只读访问整张地图，将发现的问题写入 `ValidationReport`，不在检查过程中修改地图。

问题分为 Warning、Error 和 Fatal。Fatal 表示地图结构已经无法被可靠引用或处理；存在 Fatal 或 Error 的报告不能发布为正式 `MapVersion`。

## M3_ID_UNIQUENESS：对象 ID 全局唯一性

首版 Canonical 模型要求以下九类对象共享一个全局唯一的 `ObjectId` 命名空间：

- Road；
- Lane；
- LaneBoundary；
- Junction；
- LaneConnection；
- OperationalArea；
- Station；
- RestrictedArea；
- VehicleProfile。

同类型内部重复和跨类型重复都属于 Fatal。例如，一个 Road 和一个 Lane 都使用 `shared_id`，即使某些引用字段能够根据类型推断目标，也会让日志、质检报告、变更记录和通用对象查询产生歧义，因此不允许发布。

规则按照 `MapData` 中的稳定容器顺序扫描对象。某个 ID 第一次出现时记为定义，第二次及后续每次出现各生成一个 Fatal，问题中包含：

- 稳定规则 ID `M3_ID_UNIQUENESS`；
- 重复的对象 ID；
- 第一次出现和本次出现的对象类型；
- 分配新稳定 ID 并同步引用的修复建议。

`MapHeader.map_id` 标识地图项目，不属于对象 ID 命名空间。空 ID 的格式合法性也不由本规则判断；如果多个对象都使用空 ID，它们仍会被识别为重复，空 ID 本身将在后续专门的字段合法性规则中报告。

## M3_REFERENCE_INTEGRITY：引用完整性

引用完整性规则要求 `MapData` 中每一个显式 ID 引用都能找到对应类型的目标对象。悬空引用产生 Error：地图仍能被读取并生成问题报告，但不能发布为正式版本。

当前检查范围如下：

| 持有对象 | 引用字段 | 目标类型 |
|---|---|---|
| Road | `predecessorIds`、`successorIds` | Road |
| Road | `laneIds` | Lane |
| Lane | `roadId` | Road |
| Lane | `leftBoundaryId`、`rightBoundaryId` | LaneBoundary |
| Lane | `predecessorIds`、`successorIds` | Lane |
| Junction | `connectionIds` | LaneConnection |
| LaneConnection | `junctionId` | Junction |
| LaneConnection | `incomingLaneId`、`connectingLaneId`、`outgoingLaneId` | Lane |
| Station | `accessLaneId` | Lane |
| RestrictedArea | `allowedVehicleProfileIds` | VehicleProfile |

每个悬空引用单独生成一个问题，记录稳定规则 ID `M3_REFERENCE_INTEGRITY`、持有引用的对象 ID、字段名、目标类型、目标 ID 和修复建议。一个字段中重复出现两次相同的悬空引用会生成两个问题；引用列表是否允许重复属于其他语义规则。

本规则只回答“目标是否存在”。例如 Road 包含 Lane ID、Lane 也引用该 Road 时，两侧是否互相匹配属于所有权或拓扑互反检查，不属于引用完整性。

## M3_BASIC_GEOMETRY：基础几何

基础几何规则检查地图对象是否具有可计算、可显示并可继续执行拓扑几何检查的基础形态。发现的问题产生 Error，不允许发布。

首版固定阈值如下：

| 检查项 | 阈值或约定 |
|---|---|
| Road、Lane、LaneBoundary 折线点数 | 至少 2 点 |
| 折线或轮廓总长度 | 至少 0.1 m |
| 相邻点间距 | 大于 1 mm，沿用 `points_coincident` 容差 |
| OperationalArea、RestrictedArea 轮廓 | 至少 4 点，最后一点与第一点重合 |
| 区域 XY 投影面积 | 至少 0.01 m² |
| Lane 宽度 | 1–20 m，包含边界 |
| 局部 ENU 的 X、Y | 原点 ±100 km |
| 局部 ENU 的 Z | 原点 ±10 km |
| WGS84 原点经度 | `[-180°, 180°]` |
| WGS84 原点纬度 | `[-90°, 90°]` |
| WGS84 原点高程 | ±10 km |

所有坐标和 Lane 宽度都必须是有限数字，不能包含 NaN 或正负无穷。Road 参考线、Lane 中心线、LaneBoundary 几何、两类区域轮廓和 Station 位置均纳入 ENU 坐标检查。

自相交在局部 ENU 的 XY 平面判断，相邻线段的共享端点不算自相交；闭合轮廓的首尾线段也视为相邻。非相邻线段交叉、接触或共线重叠均视为自相交。规则发现第一组非相邻相交线段后，为该对象生成一个问题，避免同一错误轮廓产生大量重复报告。

基础几何规则不判断相连 Lane 的端点距离与航向差，这些属于 M3-08 连接几何规则；也不判断区域或 Lane 的业务可通行性。

## M3_TOPOLOGY_RECIPROCITY：拓扑互反

拓扑互反规则检查目标对象已经存在时，同一关系的两侧是否表达一致。不一致产生 Error 并阻止发布。

首版检查四组双向关系：

```text
Road A.successorIds 包含 B
    ⇅
Road B.predecessorIds 包含 A

Lane A.successorIds 包含 B
    ⇅
Lane B.predecessorIds 包含 A

Road.laneIds 包含 Lane
    ⇅
Lane.roadId 指向 Road

Junction.connectionIds 包含 LaneConnection
    ⇅
LaneConnection.junctionId 指向 Junction
```

检查同时从两个方向执行：如果 A 声明 B 为后继，则 B 必须声明 A 为前驱；如果 B 声明 A 为前驱，则 A 也必须声明 B 为后继。所有权关系同样既检查父对象列表，也检查子对象的反向所属 ID。

当被引用目标不存在时，本规则跳过该关系，由 `M3_REFERENCE_INTEGRITY` 生成悬空引用问题，避免同一根因产生重复报告。ID 重复导致的目标歧义由 `M3_ID_UNIQUENESS` 先行报告。

本规则只检查拓扑声明是否双向一致，不检查相连几何的端点距离、航向和曲率。两个 Lane 即使互相声明了前驱后继，仍可能在空间上断开；该问题由 M3-08 连接几何规则处理。

## M3_CONNECTION_GEOMETRY：连接几何

连接几何规则检查拓扑相连对象在空间上能否形成连续行驶连接。不满足容差时产生 Error 并阻止发布。

首版固定容差：

| 检查项 | 容差 |
|---|---:|
| 前一对象行驶终点到后一对象行驶起点的三维距离 | ≤ 0.5 m |
| 两侧连接端的 XY 行驶航向差 | ≤ 30°，约 0.524 rad |
| 用于计算航向的端部 XY 方向线段 | ≥ 0.01 m |

Road 按 `referenceLine` 的存储方向从起点连接到 successor 起点。Lane 必须考虑 `direction`：

- `along_reference_line`：从 `centerline.front()` 行驶到 `centerline.back()`；
- `against_reference_line`：从 `centerline.back()` 行驶到 `centerline.front()`。

规则检查 Road 的 successor 连接、Lane 的 successor 连接，以及每个 LaneConnection 的 `incomingLaneId → connectingLaneId → outgoingLaneId`。如果同一 Lane 对已经由 successor 和 LaneConnection 同时表达，只检查一次，避免重复问题。

航向使用连接端向内寻找的第一段 XY 长度至少 0.01 m 的线段计算，Z 不参与航向角。如果两侧几何有效但找不到可用 XY 方向线段，规则报告“无法计算连接航向”。端点距离仍使用三维距离，因此高程突跳不会被忽略。

目标对象不存在或基础几何包含点数不足、NaN 等问题时，本规则跳过相应连接，分别交给引用完整性和基础几何规则报告。连接关系是否双向声明则由拓扑互反规则负责。

## M3_NETWORK_REACHABILITY：路网可达性

路网可达性规则验证 Warehouse 业务场景中，每个 LoadingBay Station 是否至少能由一种已定义车辆从 Gate Station 到达。不可达产生 Error 并阻止发布。

### 当前业务锚点

`OperationalArea` 只有区域轮廓，没有 `accessLaneId`，不能直接作为图搜索节点。因此当前版本使用以下映射：

```text
存在 Warehouse OperationalArea
        ↓ 激活物流园可达性检查
Gate Station.accessLaneId
        ↓ 作为仓库业务流的可路由起点
LoadingBay Station.accessLaneId
        ↓ 作为装卸月台目标
```

这与物流园 V0 中“门岗到 A1 月台”的典型路线一致。它不是在声称 Gate 的空间位置就是仓库；后续如果 Station 增加所属区域或 Warehouse 增加接入 Lane，应改为显式业务关系。

### 路网与车辆约束

规则以 Lane 为有向节点，通过两类关系建立邻接边：

- `Lane.successorIds`；
- `LaneConnection` 的 `incoming → connecting → outgoing`。

使用广度优先搜索判断是否存在路径。车辆只能进入同时满足以下条件的 Lane：

- `status == open`；
- Lane 与 VehicleProfile 的宽度均为有限正数，且 `lane.widthM >= vehicle.widthM`；
- Lane 中心线不经过禁止该车辆的 RestrictedArea。

RestrictedArea 是否影响 Lane，通过中心线点位于轮廓内部或中心线段与轮廓边相交来判断。非空 `allowedVehicleProfileIds` 作为白名单；车辆 ID 不在其中时禁止通过。空白名单的业务语义尚未冻结，当前规则不施加限制。

对每个 LoadingBay，规则遍历全部 VehicleProfile 和全部有效 Gate。只要存在一个“起点 + 车辆 + 路径”组合即可通过；某个车辆被限制但另一个车辆能够合法到达，不属于地图错误。

当前地图没有 Lane 限高、限长或显式曲率字段，因此 VehicleProfile 的高度、长度和最小转弯半径暂不参与可达性过滤。它们不能因为缺少地图侧约束而被武断判定为通过或失败。

如果引用目标不存在，本规则跳过相关 Station 或 Lane，由引用完整性规则报告；没有 Warehouse 的地图不适用物流园仓库到月台检查。该规则只验证路径存在，不计算最短路线、路线成本或输出导航结果，这些属于 M5 车端路径规划。
