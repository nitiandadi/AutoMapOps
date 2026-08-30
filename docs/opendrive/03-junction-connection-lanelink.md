# OpenDRIVE 入门 03：Junction 的 connection 与 laneLink

配套内容：

- 地图：[junction_straight_left.xodr](../../maps/opendrive/junction_straight_left.xodr)
- 检查脚本：[inspect_junction.py](../../examples/opendrive/inspect_junction.py)

## 这张地图解决什么问题

入口 `Road 1` 只有一条 `lane -1`，进入路口后可以：

```text
                                  Road 3 lane -1
                                        ↑
                                        │
                              Road 101（左转圆弧）
                                     ↗
Road 1 lane -1 ───────→ Junction 10
                                     ↘
                              Road 100（直线）
                                        │
                                        → Road 2 lane -1
```

其中：

- Road 1：入口道路；
- Road 2：直行出口道路；
- Road 3：左转出口道路；
- Road 100：路口内部的直行 Connecting Road；
- Road 101：路口内部的左转 Connecting Road；
- Junction 10：组织两条可通行路径的关系容器。

## 两条完整的车道路线

```text
直行：Road 1 lane -1 → Road 100 lane -1 → Road 2 lane -1
左转：Road 1 lane -1 → Road 101 lane -1 → Road 3 lane -1
```

注意，Junction 的 `connection/laneLink` 只负责路线的前半段：

```text
入口 Road lane → Connecting Road lane
```

Connecting Road 再使用自己的道路级、车道级 `successor` 接到出口 Road。

## connection 四个关键属性

第一条 connection：

```xml
<connection
    id="0"
    incomingRoad="1"
    connectingRoad="100"
    contactPoint="start">
    <laneLink from="-1" to="-1"/>
</connection>
```

| 属性 | 本例含义 |
|---|---|
| `id="0"` | Junction 内部的 connection 编号 |
| `incomingRoad="1"` | 车辆从 Road 1 进入路口 |
| `connectingRoad="100"` | 进入路口后走 Road 100 |
| `contactPoint="start"` | Road 100 以自己的 start 端连接入口 |

最容易混淆的是 `contactPoint`：

> `connection` 的 `contactPoint` 指 Connecting Road 的哪一个端点接入口，不是 incomingRoad 的端点。

入口 Road 1 使用自己的道路级 link 表明，它的终点进入 Junction 10：

```xml
<successor elementType="junction" elementId="10"/>
```

## laneLink 的 from 和 to 属于谁

```xml
<laneLink from="-1" to="-1"/>
```

严格对应：

```text
from = incomingRoad 的车道编号
to   = connectingRoad 在 contactPoint 端的车道编号
```

因此第一条 connection 表示：

```text
Road 1 lane -1 → Road 100 start 端的 lane -1
```

第二条表示：

```text
Road 1 lane -1 → Road 101 start 端的 lane -1
```

同一个入口车道可以出现在不同 connection 中，这正是分流拓扑：

```text
Road 1 lane -1 ─┬→ Road 100 lane -1（直行）
                 └→ Road 101 lane -1（左转）
```

## Connecting Road 如何继续接出口

以 Road 101 为例，道路级 link 为：

```xml
<link>
    <predecessor elementType="road" elementId="1" contactPoint="end"/>
    <successor elementType="road" elementId="3" contactPoint="start"/>
</link>
```

这里两个 `contactPoint` 指被连接的 Road 1 和 Road 3 使用哪个端点。

Road 101 的 lane -1 还要写：

```xml
<link>
    <predecessor id="-1"/>
    <successor id="-1"/>
</link>
```

于是完整关系才闭合：

```text
Junction laneLink：Road 1 lane -1 → Road 101 lane -1
Road 101 successor：Road 101 lane -1 → Road 3 lane -1
```

## 一个 connection 能有多个 laneLink 吗

可以。假设入口 Road 1 和 Connecting Road 100 都有两条车道，可以写：

```xml
<connection id="0" incomingRoad="1"
            connectingRoad="100" contactPoint="start">
    <laneLink from="-1" to="-1"/>
    <laneLink from="-2" to="-2"/>
</connection>
```

一个 connection 表示同一对“入口 Road → Connecting Road”，其中可以包含多组车道映射。

如果目标 Connecting Road 不同，应使用不同的 connection，而不是把目标 Road 写进 laneLink。

## Road link、connection、laneLink 的职责

| 层级 | 回答的问题 |
|---|---|
| Road 1 的 `successor → Junction 10` | 入口道路进入哪个路口？ |
| Junction 的 `connection` | 从入口道路可以选择哪条 Connecting Road？ |
| Junction 的 `laneLink` | 入口的哪条车道映射到 Connecting Road 的哪条车道？ |
| Connecting Road 的 `successor` | 穿过路口后接到哪条出口道路？ |
| Connecting Road lane 的 `successor` | 穿过路口后接到出口道路的哪条车道？ |

## 运行拓扑检查

在项目根目录执行：

```powershell
python .\examples\opendrive\inspect_junction.py
```

预期输出的核心部分：

```text
Junction 10：2 条 connection
  connection 0：Road 1 lane -1 -> Road 100(start) lane -1 -> Road 2(start) lane -1
  connection 1：Road 1 lane -1 -> Road 101(start) lane -1 -> Road 3(start) lane -1

检查通过：所有 connection 和 laneLink 引用均可解析。
```

## 用查看器观察

esmini 的 `odrviewer` 更适合看 Junction 全貌：

```powershell
odrviewer.exe --odr "F:\practice\snartdrivingLearn\maps\opendrive\junction_straight_left.xodr" --density 0 --ground_plane
```

QGIS 的 OpenDRIVE Map Viewer 可以看到各条 Road 的几何，但该插件目前对 Junction 关系本身的展示有限；不要仅凭二维图形判断 laneLink 是否正确，应结合检查脚本查看拓扑。

## 建议做的四个破坏性实验

复制一份地图后，每次只修改一处：

1. 把 connection 1 的 `to="-1"` 改成 `to="-2"`，运行脚本观察无效目标车道错误。
2. 把 connection 0 的 `contactPoint="start"` 改成 `end`，理解映射端点为什么发生变化。
3. 删除 connection 1：左转的圆弧几何仍存在，但入口车道不再拥有左转拓扑。
4. 删除 Road 101 lane -1 的车道级 `successor`：入口仍能进入左转连接道路，但无法得到完整的出口车道路线。

这四个实验用于区分：

```text
几何存在
≠ Road 连接完整
≠ Junction connection 完整
≠ laneLink 和车道 successor 完整
```

## 下一步

看懂本例后，下一步是把单车道分流扩展为：

- 一个 connection 包含多个 laneLink；
- 不同入口车道拥有不同转向权限；
- 多入口、多出口的完整十字路口；
- 检查 connection 与 Connecting Road 几何端点是否连续。
