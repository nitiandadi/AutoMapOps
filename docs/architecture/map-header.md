# MapHeader 与坐标参考

## 作用

`MapHeader` 是一份 Canonical 地图的头信息，负责标识地图项目并明确地图内部坐标的解释方式。道路、车道和场景对象不允许依赖程序中的固定原点。

## 字段

| C++ 字段 | JSON 字段 | 类型 | 含义 |
|---|---|---|---|
| `map_id` | `mapId` | `MapId` | 地图项目的稳定 ID，跨版本保持不变 |
| `name` | `name` | string | 面向用户的地图名称 |
| `schema_version` | `schemaVersion` | string | Canonical Map Model 结构版本，首版为 `1.0` |
| `coordinate_reference` | `coordinateReference` | object | 地理基准、ENU 原点和单位 |

`CoordinateReference` 首版固定约定：

| 字段 | 默认值 | 含义 |
|---|---|---|
| `geodetic_datum` | `WGS84` | 原点经纬度采用的地理基准 |
| `origin` | 显式提供 | 建立局部 ENU 坐标系的经度、纬度和高程 |
| `local_frame` | `enu` | X 向东、Y 向北、Z 向上 |
| `linear_unit` | `m` | 坐标、距离、宽度和高度使用米 |
| `angle_unit` | `rad` | 航向角使用弧度 |

## C++ 示例

```cpp
const automap::core::MapHeader header{
    .map_id = automap::core::MapId{"logistics_park_demo"},
    .name = "物流园教学地图",
    .schema_version = "1.0",
    .coordinate_reference = automap::core::CoordinateReference{
        .geodetic_datum = "WGS84",
        .origin = {
            .longitude_deg = 104.0668,
            .latitude_deg = 30.5728,
            .altitude_m = 500.0,
        },
        .local_frame = automap::core::LocalCoordinateFrame::enu,
        .linear_unit = "m",
        .angle_unit = "rad",
    },
};
```

地图中的 `Point3d{100.0, 20.0, 3.0}` 表示相对该原点向东 100 米、向北 20 米、向上 3 米，不表示经纬度。

## JSON 目标结构

M3 的 Canonical JSON 读写应将上述对象映射为：

```json
{
  "mapId": "logistics_park_demo",
  "name": "物流园教学地图",
  "schemaVersion": "1.0",
  "coordinateReference": {
    "geodeticDatum": "WGS84",
    "origin": {
      "longitudeDeg": 104.0668,
      "latitudeDeg": 30.5728,
      "altitudeM": 500.0
    },
    "localFrame": "enu",
    "linearUnit": "m",
    "angleUnit": "rad"
  }
}
```

## 校验边界

`map_core` 负责保存和比较数据，不在构造阶段拒绝错误值。经纬度范围、空 ID、不支持的 Schema 版本、坐标系和单位等问题由 M3 的 `map_validation` 统一报告。
