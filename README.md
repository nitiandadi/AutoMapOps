# AutoMapOps

AutoMapOps 是一个面向自动驾驶地图学习的 C++20 练习项目，目标是在物流园、矿区等封闭场景中跑通地图生产、质检、版本发布、车端地图包和运行时消费的最小闭环。

当前进度：M0、M1 和 M2 已完成，下一步进入 M3-01，实现 Canonical JSON 读取。

## 已实现

- CMake + C++20 多模块工程；
- `map_core`、`map_io`、`map_validation`、`map_version`；
- 可运行的 `automap_cli`；
- Canonical Map Model 分层与基础对象骨架；
- `Point3d`、`Polyline3d`、`BoundingBox3d` 及基础几何算法；
- Road、Lane、LaneBoundary 及基础拓扑、边界共享与跨越权限；
- Junction、园区场景对象、车辆通行约束和完整 `MapData` 聚合根；
- 小而完整的[物流园 Canonical V0 草稿](maps/drafts/logistics_park_v0.json)及其[中文设计说明](maps/drafts/logistics_park_v0.md)；
- 独立的 `visualizer` Web 调试项目，可分图层查看内部地图模型并检查对象字段；
- MSVC 构建与 CTest 自动测试。

## 可视化调试器

[`visualizer`](visualizer/README.md) 是独立的 TypeScript + Vite 子项目，不参与 C++ 核心构建。它可以打开 Canonical JSON，显示 Road、Lane、LaneBoundary、拓扑、业务区域、Station 和路口连接。

```powershell
cd visualizer
npm install
npm run dev
```

## 构建

请在 Visual Studio x64 开发者 PowerShell 或命令提示符中执行：

```powershell
cmake -S . -B out/build/x64-Debug -DBUILD_TESTING=ON
cmake --build out/build/x64-Debug --config Debug
ctest --test-dir out/build/x64-Debug -C Debug --output-on-failure
```

详细说明见 [构建与运行](docs/development/build-and-run.md)，项目路线图见 [任务清单](docs/project-management/tasks.md)。

## 定位与限制

本项目用于学习和求职作品演示。Canonical Map Model 是项目自定义教学格式，不是 OpenDRIVE、Lanelet2 等行业标准，也不能直接用于量产车辆。
