# Canonical 曲线与可视化改造执行记录

本文档是 `VISUALIZER_UPGRADE_PLAN.md` 的落地任务线。CV 改造独立于当前 M3 和后续 M4–M9，不改变原里程碑编号。

## 任务状态

| 状态 | ID | 工作项 | 产物或验收条件 |
|---|---|---|---|
| ✅ | CV-00 | 基线保护与计划落盘 | 保留现有 M3 改动；计划、允许修改范围和验证边界已记录 |
| ✅ | CV-01 | Canonical JSON 1.1 协议 | JSON Schema、曲线示例和生成的 TypeScript wire types 已落盘 |
| 🟡 | CV-02 | C++ 曲线几何核心 | 类型、求值、反向遍历、细分和测试代码已完成；VS 2026 编译通过，待 CTest 验证 |
| 🟡 | CV-03 | Canonical JSON 读写 | 1.0/1.1 分派、曲线读写和目标版本控制已完成；待 CTest 验证 |
| 🟡 | CV-04 | 校验、可达性与 CLI | 曲线连续性、精确连接状态、细分相交和统计已完成；待 CTest 验证 |
| ✅ | CV-05 | Vue 3 + deck.gl 等价迁移 | Vue 组件、Pinia 状态、WebGL 图层和文件加载入口完成，TypeScript 类型检查通过 |
| ✅ | CV-06 | 曲线直渲和 Worker | Worker 解析、三档 LOD、Float32 原点平移、Transferable 与二进制 PathLayer 完成 |
| ✅ | CV-07 | 标签避让和对象选择 | GPU 标签碰撞、稳定键、宽拾取层、重叠候选和对象搜索完成 |
| 🟡 | CV-08 | 集成验收与文档 | 文档、单元测试和实际文件浏览器回归完成；大型性能基准待执行 |

状态说明：`✅` 表示对应实现和本轮允许的静态验收已完成；`🟡` 表示代码已实现，但仍等待用户自行运行约定中的测试或性能验证。

## 实施结果

- 数据层：`Road.referenceLine`、`Lane.centerline`、`LaneBoundary.geometry` 支持旧点列或 `composite_curve`。
- 曲线段：支持 `line`、`circular_arc`、`clothoid`，Z 沿 XY 弧长线性插值。
- C++：提供统一求值、长度、包围盒、前后向状态和确定性细分 API。
- I/O：读取 1.0/1.1，保留输入表示；曲线写入 1.0 明确失败。
- 质检：增加曲线参数以及 G0/G1/G2 检查，连接校验使用精确曲线端点和切向。
- 前端：Vue 3 + TypeScript + Pinia，deck.gl `OrthographicView` 直接使用本地 ENU。
- 大数据：JSON 解析、Schema 校验、LOD 和路径缓冲在 Worker 中完成；顶点通过 Transferable typed array 传输。
- 可用性：标签碰撞过滤、独立拾取层、重叠候选列表、无几何对象搜索和属性检查同步。
- 二维投影：Canonical 原始 Z 保留供检查器和计算使用，GPU 渲染副本压平为 Z=0，避免正高程被正交相机近裁剪面截断。

## 验证记录

已执行：

```text
npm run generate:types  通过
npm run typecheck       通过
npm run test            4 项通过
npm run build           通过
npm run test:e2e        2 项通过（含磁盘打开 canonical_curve_demo_v1_1.json）
VS 2026 C++ Debug build 通过
```

未执行：

- CTest 与 CLI 解析：遵循用户约定，本轮只编译对应测试目标，不执行测试程序。
- 10 万对象/100 万顶点基准：按用户约定不主动运行大型性能测试。

## 用户验证命令

当前机器可使用 Visual Studio 2026 生成器；在普通 PowerShell 中：

```powershell
cmake -S . -B out/build/canonical-curve -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON
cmake --build out/build/canonical-curve --config Debug
ctest --test-dir out/build/canonical-curve -C Debug --output-on-failure
```

前端：

```powershell
cd visualizer
npm run typecheck
npm run test
npm run build
npm run test:e2e
npm run benchmark
```

## 保留边界

- 不包含 OpenDRIVE 导入、在线底图、三维视图和地图编辑。
- 区域轮廓仍为闭合点列。
- 曲线细分只生成渲染或几何算法缓存，不回写 Canonical 参数。
- 当前实现是教学与调试工具，不作为量产车辆地图组件。
