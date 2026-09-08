# MapVersion V1 质检与发布

## 目的

M3 将可编辑的 Canonical 草稿经过完整质检后冻结为不可覆盖的 `MapVersion`。当前实现是教学型文件版本库，不是数据库事务系统或量产地图发布平台。

## 发布流程

```text
Canonical 草稿
    ↓ validate_map（固定顺序执行全部 M3 规则）
ValidationReport
    ├─ 有 Fatal/Error → 阻止发布，不创建版本目录
    └─ 无 Fatal/Error → 规范化写出并计算 SHA-256
                           ↓
                    临时发布目录
                           ↓ 四个文件全部成功
                    原子改名为 V1
```

规则执行顺序为：ID 唯一性、引用完整性、基础几何、拓扑互反、连接几何、路网可达性。Warning 会记录在报告中，但不阻止发布。

## 质检报告

`validation-report.json` 包含：

- `mapId`；
- Warning、Error、Fatal 数量和 `canPublish`；
- 每个问题的 `ruleId`、`severity`、`objectId`、`reason` 和 `suggestion`。

可以只生成报告而不发布：

```powershell
automap_cli validate <map.json> <validation-report.json>
```

## 版本目录

发布命令：

```powershell
automap_cli publish <draft-map.json> <version-directory> [version-id]
```

成功后固定产生：

```text
V1/
├─ manifest.json
├─ map.json
├─ validation-report.json
└─ initial-changeset.json
```

`manifest.json` 保存版本、父版本、来源 ChangeSet、坐标参考、对象数量、质检摘要以及 `sha256:` 前缀的内容哈希。内容哈希基于紧凑确定性 Canonical JSON，而不是格式化文件的空格和换行，因此重新读取后再规范化可以得到相同哈希。

发布器不会覆盖已经存在的目标目录。四个文件先写入同级 `.publishing` 临时目录，全部成功后才改名为正式目录；失败时不会留下一个看似已发布的不完整 V1。

## 不可变性验证

`verify_map_snapshot` 会重新读取 `map.json`、确认 `mapId`，然后重新计算 Canonical 内容哈希。测试还会在发布后修改内存中的源草稿，并确认正式 `map.json` 的字节和读回对象均保持不变。

当前仓库中的正式版本来自 `maps/drafts/logistics_park_v0.json`。该草稿是 CMJ 1.0 点列地图，仍受 CMJ 1.1 读取器兼容；当前 OpenDRIVE 转换得到的 CMJ 1.1 曲线草稿尚有拓扑和连接几何 Error，因此严格按照发布门槛不能替换正式 V1。
