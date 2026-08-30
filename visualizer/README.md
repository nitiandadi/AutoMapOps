# AutoMapOps Visualizer

独立的 Canonical Map Model 可视化调试项目。它只读取地图 JSON，不依赖或修改 C++ `map_core`，用于观察几何、拓扑、业务对象和原始字段。

## 当前能力

- 显示 Road 参考线、Lane 中心线和 LaneBoundary；
- 显示 OperationalArea、RestrictedArea、Station 和路口连接；
- 显示 Lane 后继拓扑箭头；
- 分图层开关、滚轮缩放、拖动画布；
- 点击对象查看完整 Canonical 字段；
- 从本地打开 JSON，内置一份物流园示意数据。

## 运行

```powershell
cd visualizer
npm install
npm run dev
```

终端会给出本地访问地址。生产构建使用：

```powershell
npm run build
```

## JSON 约定

当前 TypeScript 接口镜像 M2 的 `MapData`，JSON 字段使用 camelCase。几何点同时接受数组和对象：

```json
[10.0, 20.0, 0.0]
```

或：

```json
{"x": 10.0, "y": 20.0, "z": 0.0}
```

这是调试器的预接入契约。M3 Canonical JSON Schema 冻结后，应以正式 Schema 为准同步这里的类型与兼容层。

## 定位与限制

本工具只做二维俯视调试，Z 坐标暂不参与绘制；它不是 GIS 编辑器，也不是 OpenDRIVE 查看器。对象引用、几何连续性和业务合法性仍由后续 `map_validation` 检查。
