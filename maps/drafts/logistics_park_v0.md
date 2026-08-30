# 物流园 Canonical V0 草稿说明

## 1. 定位与来源

本草稿以 `maps/opendrive/logistics_park_skeleton.xodr` 为几何和路线骨架，整理为 AutoMapOps M2 的 Canonical Map Model。Canonical JSON 是本项目的教学型内部格式，不是 OpenDRIVE、Lanelet2 或可直接用于量产车辆的高精地图格式。

V0 保留骨架中的三个核心路线意图：

- 主线路径：入口路 → 分流口 → 主分拨路 → 出口段；
- 绕行路径：入口内侧车道 → 分流口左转 → 装卸区绕行路 → 出口内侧车道；
- 回程路径：出口段 → 南侧回程闭环 → 入口路。

为满足 M2 “8～12 条 Lane、一个路口”的小型教学范围，OpenDRIVE 骨架的 J10 被保留为唯一显式 `Junction`；J20 的简单汇流连接几何被并入 `road_main` 和 `road_detour` 的末端。汇流关系通过 `lane_exit_inner` 同时引用 `lane_main_inner` 与 `lane_detour` 两个前驱来表达，不再建立第二个 `Junction`。

## 2. 坐标说明

### 2.1 坐标参考

| 项目 | V0 约定 |
|---|---|
| 地理基准 | WGS84 |
| 虚构教学原点 | 经度 104.0668°、纬度 30.5728°、高程 500.0 m |
| 局部坐标系 | ENU |
| X 轴 | 向东为正 |
| Y 轴 | 向北为正 |
| Z 轴 | 向上为正，本草稿全部为 0.0 m |
| 线性单位 | m |
| 角度单位 | rad |

JSON 中的 `[x, y, z]` 是相对上述 WGS84 原点的局部 ENU 坐标。例如 `[110.0, 78.25, 0.0]` 表示原点以东 110 m、以北 78.25 m、同高程处，不是经纬度。

### 2.2 几何取值

- 主路参考线沿 `Y=0` 从西向东；两条右侧车道中心线分别位于参考线右侧 1.75 m 和 5.25 m。
- 所有 Lane 宽度均为 3.5 m，边界位于距参考线 0 m、3.5 m、7.0 m 的位置。
- 绕行路采用离散折线近似 OpenDRIVE 的直线和圆弧；折线点越多只代表离散近似更细，不表示新增拓扑节点。
- 南侧回程闭环保留骨架中半径约 50 m 的四次右转形状，用离散点表达。
- Road 参考线、Lane 中心线和 LaneBoundary 点序都沿车辆行驶方向；本图全部 Lane 均为 `along_reference_line`。

## 3. 对象清单

### 3.1 Road 与 Lane

| Road | 用途 | Lane |
|---|---|---|
| `road_entry` | 门岗至分流口，双车道 | `lane_entry_inner`、`lane_entry_outer` |
| `road_j10_main_connector` | J10 双车道直行连接 | `lane_j10_main_inner`、`lane_j10_main_outer` |
| `road_j10_detour_connector` | J10 单车道左转连接 | `lane_j10_detour` |
| `road_main` | 可作为后续动态封闭目标的主分拨路 | `lane_main_inner`、`lane_main_outer` |
| `road_detour` | 经装卸区的低速绕行路线及末端汇流 | `lane_detour` |
| `road_exit` | 汇流后的双车道出口段 | `lane_exit_inner`、`lane_exit_outer` |
| `road_return` | 返回入口的双车道闭环 | `lane_return_inner`、`lane_return_outer` |

合计 7 条 Road、12 条 Lane。内外侧 Lane 的 `orderFromReference` 分别为 1 和 2，全部位于 Road 参考线右侧，符合该单向 RHT 教学场景。

### 3.2 边界

- 双车道路段使用三条边界：参考线侧为不可跨越实线，中间为允许跨越虚线，外侧为不可跨越路缘石。
- 单车道绕行路两侧均不可跨越，以突出窄通道和装卸作业区约束。
- 相邻车道共享中间边界对象，例如入口两车道共同引用 `boundary_entry_middle`。
- `type` 只描述边界形态，`crossingAllowed` 单独表达静态跨越权限。

### 3.3 Junction 与 LaneConnection

唯一显式路口为 `junction_route_split`，包含三个允许动作：

| 入口 Lane | 连接 Lane | 出口 Lane | 动作 |
|---|---|---|---|
| `lane_entry_inner` | `lane_j10_main_inner` | `lane_main_inner` | 直行 |
| `lane_entry_outer` | `lane_j10_main_outer` | `lane_main_outer` | 直行 |
| `lane_entry_inner` | `lane_j10_detour` | `lane_detour` | 左转进入装卸区绕行路 |

入口外侧车道没有进入绕行路的 `LaneConnection`，因此路由器不能自行推断该动作合法。车辆若要选择绕行路线，应在进入分流口前位于 `lane_entry_inner`。

### 3.4 业务对象

| 类别 | 对象 | 说明 |
|---|---|---|
| Warehouse | `area_warehouse_a` | A 仓库建筑/业务范围 |
| LoadingArea | `area_loading_a` | A 仓北侧绕行路旁的装卸作业区 |
| ParkingArea | `area_parking` | 南侧回程区域旁的待命停车区 |
| ChargingArea | `area_charging` | 南侧回程区域旁的新能源车充电区 |
| Gate Station | `station_gate` | 接入 `lane_entry_inner` 的门岗目标点 |
| LoadingBay Station | `station_loading_a1` | 接入 `lane_detour` 的 A1 月台目标点 |
| Parking Station | `station_parking_01` | 接入 `lane_return_inner` 的待命车位目标点 |
| Charging Station | `station_charger_01` | 接入 `lane_return_inner` 的充电目标点 |
| RestrictedArea | `restricted_narrow_passage` | 装卸区北向窄通道，仅配送厢式车白名单可进入 |

`Station.position` 表示业务目标点，`accessLaneId` 表示路由到该目标时使用的接入车道；目标点不要求严格落在车道中心线上。

### 3.5 车辆约束

- `vehicle_delivery_van`：宽 2.1 m、高 2.8 m、长 5.5 m、最小转弯半径 6.0 m，可进入窄通道。
- `vehicle_truck_12m`：宽 2.55 m、高 4.0 m、长 12.0 m、最小转弯半径 10.5 m，不在窄通道白名单内。

本草稿只保存约束事实；车辆能否通过某段几何，应由后续 `map_validation` 或路由规则判断。

## 4. 拓扑说明

### 4.1 道路级拓扑

```text
road_return ──► road_entry
                  ├──► road_j10_main_connector ──► road_main ──┐
                  └──► road_j10_detour_connector ─► road_detour ├──► road_exit ──► road_return
                                                               ┘
```

`road_main` 与 `road_detour` 都是 `road_exit` 的前驱。道路级拓扑表达路线骨架，真正的可行驶分支以 Lane 和 LaneConnection 为准。

### 4.2 车道级拓扑

```text
内侧闭环主线：
lane_return_inner
  └─► lane_entry_inner
       ├─► lane_j10_main_inner ─► lane_main_inner ─┐
       └─► lane_j10_detour ─────► lane_detour ─────┴─► lane_exit_inner ─► lane_return_inner

外侧闭环主线：
lane_return_outer
  └─► lane_entry_outer ─► lane_j10_main_outer ─► lane_main_outer
       ─► lane_exit_outer ─► lane_return_outer
```

所有 `predecessorIds` 与 `successorIds` 均按双向引用填写。`lane_exit_inner` 的两个前驱体现主路与绕行路汇流；外侧车道只贯通主路，不接收绕行车辆。

### 4.3 典型路由

- 门岗到 A1 月台：`lane_entry_inner` → `lane_j10_detour` → `lane_detour` → `station_loading_a1`。
- 门岗沿主路到出口：入口内/外侧车道 → 对应 J10 直行连接车道 → 对应主路车道 → 对应出口车道。
- 月台返回门岗：`lane_detour` → `lane_exit_inner` → `lane_return_inner` → `lane_entry_inner`。
- 主路中段临时封闭时：路由规则可关闭 `lane_main_inner` 与 `lane_main_outer`，入口内侧车道仍可通过装卸区绕行路到达出口；外侧车道必须提前换入内侧车道。

## 5. V0 教学限制

- 几何由离散折线近似，未保存 OpenDRIVE 的 `line`、`arc` 曲率参数，也未引入 `spiral`。
- J20 被简化为普通 Lane 汇流，不表达让行、冲突区或优先权。
- 未表达信号灯、停止线、交通标志、车道标线颜色、分段宽度、坡度和超高。
- `RestrictedArea` 只使用车辆白名单，不表达时间窗、载重、任务状态等动态准入条件。
- 业务区域和站点均为虚构教学数据，不代表真实园区测绘成果。
- V0 适合学习 Canonical 对象、引用和路由拓扑，不可直接用于量产车辆或安全关键决策。
