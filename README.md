# AutoMapOps

AutoMapOps 是一个面向自动驾驶地图学习的 C++20 练习项目，目标是在物流园、矿区等封闭场景中跑通地图生产、质检、版本发布、车端地图包和运行时消费的最小闭环。

当前进度：M0、M1 和 M2-01～M2-08 已完成，正在设计物流园 V0 道路拓扑。

## 已实现

- CMake + C++20 多模块工程；
- `map_core`、`map_io`、`map_validation`、`map_version`；
- 可运行的 `automap_cli`；
- Canonical Map Model 分层与基础对象骨架；
- `Point3d`、`Polyline3d`、`BoundingBox3d` 及基础几何算法；
- Road、Lane、LaneBoundary 及基础拓扑、边界共享与跨越权限；
- Junction、园区场景对象、车辆通行约束和完整 `MapData` 聚合根；
- MSVC 构建与 CTest 自动测试。

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
