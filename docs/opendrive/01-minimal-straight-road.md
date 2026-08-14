# OpenDRIVE 入门 01：最简单的直路

配套数据：[minimal_straight_road.xodr](../../maps/opendrive/minimal_straight_road.xodr)

## 这个地图描述了什么

这是一个局部笛卡尔坐标系中的简单教学地图：

- 道路参考线从 `(0, 0)` 延伸到 `(100, 0)`，长度为 100 米。
- 参考线朝 x 轴正方向，航向角 `hdg = 0` 弧度。
- 参考线右侧只有一条可行驶车道，编号为 `-1`。
- 车道全程宽 3.5 米。
- 参考线和车道外边界均使用实线。
- 没有路口、信号灯、限速、前后道路连接或高程变化。

俯视示意图：

```text
 y（t 正方向）
 ↑
 │
 │  reference line / lane 0，y = 0
 ├────────────────────────────────────────→ x（s 方向）
 │
 │              lane -1
 │            driving lane
 │
 └────────────────────────────────────────  y = -3.5 m

 s=0                                      s=100 m
```

注意：`lane -1` 的几何中心大约位于 `t = -1.75 m`，但 OpenDRIVE 并没有直接存储这条中心线。它由参考线和车道宽度计算得到。

## 先认识五层骨架

```text
OpenDRIVE
└── road
    ├── planView
    │   └── geometry
    │       └── line
    └── lanes
        └── laneSection
            ├── center
            │   └── lane id="0"
            └── right
                └── lane id="-1"
```

对应关系：

| XML 元素 | 本例含义 |
|---|---|
| `OpenDRIVE` | 整张地图的根节点 |
| `road` | 一条 100 米长的道路 |
| `planView` | 道路参考线的平面几何 |
| `geometry` | 从 `s=0` 开始的一段几何 |
| `line` | 几何类型为直线 |
| `lanes` | 这条道路的全部车道定义 |
| `laneSection` | 从 `s=0` 开始，车道结构保持不变 |
| `lane id="0"` | 参考线位置，不可行驶 |
| `lane id="-1"` | 参考线右侧第一条车道 |
| `width` | 车道宽度多项式 |
| `roadMark` | 车道边线样式 |

## 用 esmini 打开

在 PowerShell 中，把 `odrviewer.exe` 的路径替换成你的实际安装路径：

```powershell
& "你的 esmini 路径\bin\odrviewer.exe" `
  --odr "F:\practice\snartdrivingLearn\maps\opendrive\minimal_straight_road.xodr" `
  --window 60 60 1000 600 `
  --density 0 `
  --ground_plane
```

PowerShell 用反引号 `` ` `` 续行；你之前命令中的 `^` 是 `cmd.exe` 的续行符。如果不想区分，可以写成一行：

```powershell
& "你的 esmini 路径\bin\odrviewer.exe" --odr "F:\practice\snartdrivingLearn\maps\opendrive\minimal_straight_road.xodr" --window 60 60 1000 600 --density 0 --ground_plane
```

## 建议做的三个小实验

每次只改一处，然后重新用 `odrviewer` 打开：

1. 把 `road` 和 `geometry` 的 `length` 同时从 `100.0` 改为 `200.0`，观察道路长度。
2. 把车道宽度的 `a` 从 `3.5` 改为 `5.0`，观察道路宽度。
3. 把 `<line/>` 改为 `<arc curvature="0.01"/>`，观察直路如何变成圆弧。

第三个实验中，`curvature="0.01"` 表示曲率为 `0.01 m⁻¹`，对应半径 `R = 1 / curvature = 100 m`。

## 这个示例刻意省略了什么

真实 OpenDRIVE 通常还会包含：

- 多段 `geometry`，包括直线、圆弧、螺旋线或参数曲线；
- 多个 `laneSection` 和多条车道；
- 道路及车道的前驱、后继关系；
- 高程、横坡、超高；
- 路口、交通信号、道路对象和限速等语义。

先看懂本例的“参考线 + 车道截面”关系，再逐步增加这些内容。
