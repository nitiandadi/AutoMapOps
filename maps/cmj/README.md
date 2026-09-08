# CMJ 地图示例

`CMJ` 是本项目对 `Canonical Map JSON` 的固定简称。这里保存由其他地图格式
转换得到的 CMJ 示例；源文件应继续保留，不要用转换结果覆盖源数据。

## odrviewer_v1_1

- 源文件：`../opendrive/odrviewer.xodr`
- CMJ 文件：`odrviewer_v1_1.cmj.json`
- 转换报告：`odrviewer_v1_1.conversion.json`
- 坐标：WGS84 基准下的局部 ENU，平面与高程单位均为米

生成命令：

```powershell
python examples/opendrive/opendrive_to_cmj.py `
  maps/opendrive/odrviewer.xodr `
  maps/cmj/odrviewer_v1_1.cmj.json `
  --map-id odrviewer `
  --name "RoadRunner 城市道路示例" `
  --report maps/cmj/odrviewer_v1_1.conversion.json
```

转换器保留 Road 的直线、圆弧和缓和曲线参数。Lane 与 LaneBoundary 根据
`laneOffset`、分段 `width` 和 `elevation` 生成三维采样点列，从而正确表达
可变宽度车道。Junction 的 `connection` 和 `laneLink` 转换为 CMJ
`LaneConnection` 及 Lane 前驱后继关系。

CMJ 1.1 目前只保存可行驶车道，不单独建模 sidewalk、shoulder、parking 和
OpenDRIVE `type="none"` 车道；这些车道的宽度仍参与横向偏移计算。标线颜色、
材质、精确宽度和用户自定义 `userData` 也不会完整进入 CMJ。详细数量与降级项
以转换报告为准。
