# C++ 工程构建与运行

## 环境

- Windows；
- Visual Studio，安装“使用 C++ 的桌面开发”和 CMake 工具；
- CMake 3.20或更高版本；
- 支持C++20的MSVC编译器。

## Visual Studio打开方式

在Visual Studio中选择：

```text
文件 → 打开 → 文件夹
```

打开项目根目录：

```text
F:\practice\snartdrivingLearn
```

Visual Studio会发现根目录的`CMakeLists.txt`并自动配置项目。

## 命令行构建

先打开“Developer PowerShell for VS”，进入项目目录：

```powershell
Set-Location F:\practice\snartdrivingLearn
```

配置：

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
```

编译全部目标：

```powershell
cmake --build build
```

## 测试

```powershell
ctest --test-dir build --output-on-failure
```

测试源码按照任务清单中的里程碑分组：

```text
tests/
├─ m1/   # 最小工程与模块联通
├─ m2/   # Canonical 核心模型
└─ m3/   # JSON IO、CLI 和质检规则
```

CTest 名称采用相同的阶段前缀，可以只运行某个里程碑：

```powershell
ctest --test-dir build -R "automap.m3" --output-on-failure
```

预期至少看到：

```text
automap.m1.smoke ... Passed
100% tests passed
```

## 运行CLI

当前使用NMake单配置生成器时：

```powershell
.\build\cpp\apps\automap_cli\automap_cli.exe
```

预期输出：

```text
AutoMapOps 0.1.0
Canonical format: AutoMapOps Canonical JSON
用法：
  automap_cli inspect <canonical-json-path>
```

## 查看地图摘要

`inspect` 命令读取 Canonical JSON，并输出地图 ID、名称、Schema 版本、WGS84/ENU 坐标参考、局部 ENU 几何范围和九类对象统计：

```powershell
.\build\cpp\apps\automap_cli\automap_cli.exe inspect .\maps\drafts\logistics_park_v0.json
```

Visual Studio 多配置生成器通常需要在可执行文件路径中增加 `Debug` 或 `Release` 目录。读取失败返回退出码 1；命令或参数错误返回退出码 2。

如果Visual Studio使用多配置生成器，可执行文件通常位于`out/build/<配置>/cpp/apps/automap_cli/`或带`Debug`、`Release`子目录的位置，以Visual Studio输出窗口显示的构建路径为准。

## 常用单目标构建

```powershell
cmake --build build --target automap_map_core
cmake --build build --target automap_map_io
cmake --build build --target automap_map_validation
cmake --build build --target automap_map_version
cmake --build build --target automap_cli
cmake --build build --target automap_smoke_test
```

`build/`和Visual Studio生成的`out/build/`都是构建产物目录，不要在里面修改源代码。
