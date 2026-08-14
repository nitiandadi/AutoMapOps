# OpenDRIVE 入门 02：弯道、多车道与车道展宽

配套内容：

- 地图：[curved_multilane_road.xodr](../../maps/opendrive/curved_multilane_road.xodr)
- 检查脚本：[inspect_xodr.py](../../examples/opendrive/inspect_xodr.py)

## 场景概览

这张地图比第一个直路示例增加了五个概念：

1. 一条道路由多个 `geometry` 首尾连接；
2. 使用 `arc` 表示圆弧；
3. 左右两侧都有可行驶车道；
4. 使用两个 `laneSection` 改变道路的车道结构；
5. 使用多个 `width` 多项式让新车道平滑出现。

平面结构示意：

```text
s=0                         s=80                         s=200

       lane +1                 lane +1
  ← ← ← ← ← ← ← ←        ← ← ← ← ← ← ← ←
====================      =========================
  reference line     ────────╮
====================          ╰─────────────────────
  → → lane -1 → →              → → lane -1 → →
                               - - - - - - - - - - -
                                lane -2 逐渐展宽到 3.5 m
```

`lane +1` 位于参考线左侧，标准行驶方向与道路的 `s` 方向相反；`lane -1` 和 `lane -2` 位于右侧，标准行驶方向与 `s` 方向相同。这是右侧通行道路 `rule="RHT"` 的典型结构。

## 参考线：直线接圆弧

第一段：

```xml
<geometry s="0.0" x="0.0" y="0.0" hdg="0.0" length="80.0">
    <line/>
</geometry>
```

第二段：

```xml
<geometry s="80.0" x="80.0" y="0.0" hdg="0.0" length="120.0">
    <arc curvature="0.008"/>
</geometry>
```

圆弧半径为：

```text
R = 1 / curvature = 1 / 0.008 = 125 m
```

120 米圆弧产生的航向角变化约为：

```text
Δhdg = length × curvature
     = 120 × 0.008
     = 0.96 rad
     ≈ 55°
```

## 为什么在 s=80 增加 laneSection

`laneSection` 表示一段车道拓扑结构稳定的道路：

```text
s=0~80:    lane +1 | lane 0 | lane -1
s=80~200:  lane +1 | lane 0 | lane -1 | lane -2
```

当车道数量发生变化时，需要开始新的 `laneSection`。继续存在的 `lane +1`、`lane 0` 和 `lane -1` 使用 `predecessor/successor` 建立跨截面的对应关系。

新出现的 `lane -2` 没有前驱车道。

## lane -2 如何平滑展宽

OpenDRIVE 的车道宽度不是一个固定数字，而是局部三次多项式：

```text
width(ds) = a + b·ds + c·ds² + d·ds³
```

本例前 40 米使用：

```xml
<width
    sOffset="0.0"
    a="0.0"
    b="0.0"
    c="0.0065625"
    d="-0.000109375"/>
```

得到：

| 距离 laneSection 起点 | lane -2 宽度 |
|---:|---:|
| 0 m | 0.000 m |
| 10 m | 0.547 m |
| 20 m | 1.750 m |
| 30 m | 2.953 m |
| 40 m | 3.500 m |

40 米后启用第二条 `width` 记录，保持 3.5 米：

```xml
<width sOffset="40.0" a="3.5" b="0.0" c="0.0" d="0.0"/>
```

## 在 OpenDRIVE Map Viewer 中观察

通过以下入口打开：

```text
矢量 → OpenDRIVE Viewer → Open...
```

加载后在 `ODR_curved_multilane_road` 组中找到 `lanes`，右键选择“缩放到图层”。因为本例仍使用局部坐标而不是现实经纬度，插件不会自动把视图移动到道路位置。

重点观察：

- `reference_lines`：前 80 米直线，随后进入左转圆弧；
- `lanes`：圆弧开始位置增加了 `lane -2`；
- `reference_line_segments`：分别对应 `line` 和 `arc` 两段参数化几何。

QGIS 插件是二维查看器，高程变化需要使用 esmini 等三维工具观察。

## 运行检查脚本

在项目根目录执行：

```powershell
python .\examples\opendrive\inspect_xodr.py
```

脚本会列出道路、参考线几何、车道截面，并计算 `lane -2` 在展宽过程中的宽度。

## 建议动手玩的实验

每次复制一份地图，只改一个变量：

1. 把圆弧曲率从 `0.008` 改成 `-0.008`，观察左转变成右转。
2. 把曲率改成 `0.016`，观察半径从 125 米缩小到 62.5 米。
3. 把 `lane -2` 的第二条 `width` 的 `a` 改成 `4.0`，观察 40 米处是否发生宽度跳变。
4. 删除第二个 `laneSection` 中 `lane -1` 的 `<predecessor>`，比较几何显示与拓扑信息的区别。
5. 把 `lane -1` 与 `lane -2` 之间的 `roadMark` 从 `broken` 改为 `solid`。

第 3 个实验会故意制造不连续数据，适合直观看出“文件能解析”并不等于“道路模型合理”。

## 仍然省略的内容

本例还没有：

- 多条 `road` 之间的连接；
- 十字路口和 `junction`；
- 信号灯、停止线和道路对象；
- `spiral` 缓和曲线；
- 超高和横坡。

下一步适合学习“多条 Road 的前驱/后继关系”，然后再进入路口。
