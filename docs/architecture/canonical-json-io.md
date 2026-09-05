# Canonical JSON 读写

## 作用与边界

`map_io` 负责在 Canonical JSON 与 `MapData` 之间做无损转换。Canonical JSON 是 AutoMapOps 的教学型内部格式，不是 OpenDRIVE、Lanelet2 等行业标准，也不能直接作为量产车辆地图格式。

Canonical JSON 表示测绘数据经过定位解算、要素提取、融合和人工编辑等初步成图处理后形成的内部地图快照，不表示测绘车采集的原始点云、图像、GNSS、IMU 或轨迹数据。真实系统中的 Survey MapSet、第三方地图和众包更新应先经过各自的导入、融合或审核流程，再统一转换为 `MapData`；Canonical JSON 只是 `MapData` 的一种持久化与交换适配器。

读取阶段只检查 JSON 语法、字段类型、必填字段、枚举可表示性和可选的预期地图 ID。ID 唯一性、引用完整性、几何合理性和拓扑合法性仍由后续 `map_validation` 处理，因此读入成功不代表地图已经通过质检。

## 公共接口

头文件：`automap/io/canonical_json.hpp`

```cpp
#include "automap/io/canonical_json.hpp"

const auto map = automap::io::read_canonical_json_file(
    "maps/drafts/logistics_park_v0.json",
    automap::io::CanonicalJsonReadOptions{
        .expected_map_id = automap::core::MapId{"logistics_park"},
    });

automap::io::write_canonical_json_file(
    "map-copy.json",
    map,
    automap::io::CanonicalJsonWriteOptions{
        .pretty_print = true,
        .indent_spaces = 2,
    });
```

同时提供 `read_canonical_json()` 和 `write_canonical_json()` 完成内存字符串转换。

## 读取约定

- 默认拒绝未知字段，避免拼写错误被静默忽略；需要读取带扩展字段的数据时，可将 `reject_unknown_fields` 设为 `false`；
- 所有当前模型必填字段都必须存在，车辆高度、长度和最小转弯半径允许缺失或为 `null`；
- 三维 ENU 点采用 `[x, y, z]`，必须恰好有三个数字；
- 对象数组和 ID 数组保持 JSON 原有顺序，以满足 `MapData` 的稳定值比较；
- 支持 UTF-8 文本、JSON Unicode 转义和可选 UTF-8 BOM；
- 语法错误、类型错误、未知字段和枚举错误统一抛出 `CanonicalJsonError`，其中 `json_path()` 给出类似 `$.lanes[0].widthM` 的定位路径。

## 写出约定

- 字段顺序由写出器固定，对象和 ID 数组顺序保持不变；
- 默认输出 2 空格缩进并在文件末尾保留换行，也可选择无排版换行的紧凑格式；
- 浮点数使用足以无损读回 `double` 的精度；
- `NaN` 和正负无穷不能由 JSON 表示，写出时会抛出 `CanonicalJsonError`；
- 未设置的车辆可选尺寸统一写为 `null`。

## 往返保证

在模型只包含可写出的有限浮点数和有效枚举值时：

```text
MapData → Canonical JSON → MapData
```

读回结果与原始 `MapData` 按值完全一致，包括对象顺序、拓扑 ID 顺序、中文文本、可选字段和浮点值。该保证是 M3-02 的验收标准，也是后续稳定内容哈希与 MapVersion 发布的基础。
# Canonical JSON 1.1 曲线路径补充

Canonical JSON 1.1 在保持 1.0 点列兼容的基础上，为 `Road.referenceLine`、`Lane.centerline` 和 `LaneBoundary.geometry` 增加 `composite_curve` 表示。区域轮廓仍为闭合点列。

曲线段按数组顺序连接，支持：

- `line`：起点、起始航向、XY 弧长、终点 Z；
- `circular_arc`：在 line 参数基础上增加常曲率；
- `clothoid`：在 line 参数基础上增加起止曲率，曲率沿弧长线性变化。

统一约定：

- XY 使用地图头声明的局部 ENU 米制坐标；
- 航向单位为弧度，0 指向 +X，逆时针为正；
- 曲率单位为 `1/m`，正值左转、负值右转；
- `lengthM` 是 XY 平面弧长；
- Z 从 `start[2]` 到 `endZM` 按 XY 弧长线性插值。

正式机器可读协议见 `schemas/canonical-map-1.1.schema.json`，连续曲线示例见 `maps/drafts/canonical_curve_demo_v1_1.json`。

兼容策略：

- 1.0 只接受点列路径；
- 1.1 同时接受点列和组合曲线；
- 读取器拒绝未知 Schema 版本；
- 写出默认保留 `MapHeader.schema_version`；
- 曲线写入目标版本 1.0 时明确失败，不执行隐式离散化。
