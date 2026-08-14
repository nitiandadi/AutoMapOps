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
```

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
