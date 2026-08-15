# Junction 与 LaneConnection 路口模型

## 职责划分

`Junction` 是路口容器，保存名称及其允许通行动作的 ID。`LaneConnection` 表示一条明确允许的车道级转向关系：

```text
incoming_lane_id
        ↓
connecting_lane_id（路口内部，几何属于普通 Lane）
        ↓
outgoing_lane_id
```

一条 `LaneConnection` 的存在即表示该动作在静态地图中合法；不存在的组合不能由路由器自行猜测。`junction_id` 建立从连接到所属路口的反向引用，`Junction.connection_ids` 建立正向所有权引用。这两侧是否互相匹配由 M3 质检负责。

## 转向类型

`TurnDirection` 提供 `straight`、`left`、`right` 和 `u_turn` 四个稳定序列化名称。它用于展示、规则和路由代价；真正可行驶的形状仍以 `connecting_lane_id` 对应 Lane 的中心线、边界和前后继为准。

## 与 OpenDRIVE 的关系

导出 OpenDRIVE 时，`Junction` 可转换为 `<junction>`，`LaneConnection` 的入口、连接和出口关系用于生成 `<connection>` 与 `<laneLink>`。本模型直接使用车道 ID，而 OpenDRIVE 需要结合 Road、laneSection、正负 lane ID 和 `contactPoint` 重建目标结构，因此不是逐字段复制。

## 首版限制

当前不表达信号相位、冲突区、优先权、停止让行、车辆分类转向权限和多段内部连接。这些属于后续交通规则与复杂 Junction 扩展。
