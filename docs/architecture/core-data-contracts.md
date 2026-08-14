# AutoMapOps 核心数据契约设计

## 1. 文档目的

本文定义 AutoMapOps 在地图生产、版本发布、地图编译和车端运行过程中使用的五类核心数据契约：

1. `CandidateMapPatch`：车端或建图算法提出的候选地图变更；
2. `ChangeSet`：平台审核后形成的正式变更集合；
3. `MapVersion`：不可变的正式逻辑地图版本；
4. `MapPackage`：由正式地图版本编译得到的车端运行制品；
5. `DynamicEvent`：不修改正式地图版本的临时运行事件。

这些对象是项目自定义的教学与练手数据契约，不是 OpenDRIVE、Lanelet2、ADASIS 等行业标准。后续可以为标准格式编写导入器或导出器，但平台内部语义以本文为准。

## 2. 总体数据生命周期

```text
车端或建图算法
  │
  └─ CandidateMapPatch
          │
          ▼
平台接收、对比、修订和审核
          │
          └─ ChangeSet
                  │
                  ▼
            MapVersion V2
                  │
                  ▼
              MapPackage
                  │
                  ▼
                车端

平台 ── DynamicEvent ──► 车端运行时覆盖层
```

首版地图没有候选 Patch，冷启动关系为：

```text
工作草稿 V0
→ initial ChangeSet
→ MapVersion V1
→ MapPackage V1
```

## 3. 公共约定

### 3.1 字段命名和序列化

- 文档中的类型名使用 `PascalCase`；
- JSON 字段使用 `camelCase`；
- 枚举值使用小写 `snake_case`；
- 时间使用带时区的 ISO 8601 字符串，例如 `2026-08-06T10:30:00+08:00`；
- ID 使用稳定字符串，不将数据库自增主键暴露为业务 ID；
- Hash 使用带算法前缀的形式，例如 `sha256:abc123...`；
- 所有契约必须包含 `schemaVersion`，用于兼容后续结构升级；
- JSON 示例仅用于表达契约，正式 C++ 实现应提供对应类型和严格校验。

### 3.2 公共标识符

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `schemaVersion` | string | 是 | 当前数据契约版本，例如 `1.0` |
| `mapId` | string | 是 | 地图项目的稳定 ID，例如 `logistics_park_demo` |
| `createdAt` | datetime | 是 | 对象首次创建时间 |
| `createdBy` | string | 是 | 创建者，可以是用户、车辆、算法或系统服务 ID |

地图业务对象使用稳定 ID，例如：

```text
road_main_01
lane_main_01_forward
junction_gate_01
station_warehouse_a
area_loading_01
```

同一个对象跨地图版本保持相同 ID。对象被删除后，其 ID 不得重新分配给另一个语义对象。

### 3.3 地图对象引用

五类契约引用地图对象时统一使用：

```json
{
  "objectType": "lane",
  "objectId": "lane_main_01_forward"
}
```

首版允许的 `objectType`：

```text
road
lane
lane_boundary
junction
lane_connection
operational_area
station
restricted_area
vehicle_profile
```

### 3.4 坐标和几何

本文只规定所有几何必须关联明确的坐标参考，具体原点、轴方向和容差由 M0-07 的坐标系统约定文档定义。

任何带几何的契约至少需要能够追溯：

- `mapId`；
- 使用的地图版本；
- 坐标系类型；
- 坐标单位；
- 局部坐标原点或对应地图头信息。

禁止在不知道坐标系的情况下直接合并两份候选几何。

## 4. CandidateMapPatch

### 4.1 定义

`CandidateMapPatch` 是车端、众包建图模块、变化检测算法或模拟器针对局部地图变化生成的待审核提案。

它具有以下性质：

- 是候选数据，不是正式地图；
- 可能包含误检或不完整拓扑；
- 必须说明基于哪个正式地图版本产生；
- 不能直接修改 `MapVersion`；
- 通过平台审核后才能转换为 `ChangeSet`。

### 4.2 数据方向

```text
车端 / 建图算法 / Patch 模拟器
                ↓
          CandidateMapPatch
                ↓
              平台
```

### 4.3 字段

| 字段 | 类型 | 必填 | 写入方 | 含义 |
|---|---|---:|---|---|
| `schemaVersion` | string | 是 | 产生方 | Patch 契约版本 |
| `patchId` | string | 是 | 产生方 | 全局唯一 Patch ID |
| `mapId` | string | 是 | 产生方 | 目标地图项目 |
| `baseMapVersion` | string | 是 | 产生方 | 产生候选变化时车辆使用的正式地图版本 |
| `source` | object | 是 | 产生方 | 来源类型和来源 ID |
| `observedAt` | datetime | 是 | 产生方 | 现场观测时间 |
| `receivedAt` | datetime | 是 | 平台 | 平台接收时间 |
| `confidence` | number | 是 | 产生方 | 整体置信度，范围 `[0, 1]` |
| `operations` | array | 是 | 产生方 | 一个或多个候选变更操作 |
| `evidence` | object | 否 | 产生方 | 观测次数、传感器摘要等证据信息 |
| `status` | enum | 是 | 平台 | Patch 当前处理状态 |
| `review` | object | 否 | 平台 | 审核人、审核意见及时间 |
| `resultingChangeSetId` | string | 否 | 平台 | 接受后产生的 ChangeSet ID |

`source.type` 首版支持：

```text
vehicle
mapping_algorithm
manual_import
simulator
```

单条 `operations` 元素包含：

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `operationId` | string | 是 | Patch 内唯一操作 ID |
| `action` | enum | 是 | `create`、`update` 或 `delete` |
| `target` | ObjectReference | 是 | 目标对象类型和 ID |
| `candidateValue` | object/null | 是 | 候选对象内容；删除操作为 `null` |
| `changedFields` | array | 否 | 更新操作涉及的字段路径 |
| `topologySuggestions` | array | 否 | 候选前驱、后继或转向关系 |
| `confidence` | number | 是 | 当前操作的置信度 |

### 4.4 状态机

```text
received
   ↓
reviewing
   ├─► accepted
   ├─► rejected
   ├─► conflicted
   └─► superseded
```

| 状态 | 含义 |
|---|---|
| `received` | 平台已经接收，尚未开始审核 |
| `reviewing` | 正在与基准版本比较或人工修订 |
| `accepted` | 已接受，并已经产生 ChangeSet |
| `rejected` | 被确认无效或不应进入正式地图 |
| `conflicted` | 基准版本过旧或与其他变更冲突，不能直接接受 |
| `superseded` | 已被更新、更完整的 Patch 替代 |

`accepted` 是候选流程终态，不表示地图已经发布。只有对应 ChangeSet 被提交后才会产生新 `MapVersion`。

### 4.5 最小示例

```json
{
  "schemaVersion": "1.0",
  "patchId": "patch_20260806_001",
  "mapId": "logistics_park_demo",
  "baseMapVersion": "V1",
  "source": {
    "type": "vehicle",
    "id": "vehicle_12"
  },
  "observedAt": "2026-08-06T09:20:00+08:00",
  "receivedAt": "2026-08-06T09:20:10+08:00",
  "confidence": 0.87,
  "operations": [
    {
      "operationId": "op_001",
      "action": "create",
      "target": {
        "objectType": "lane",
        "objectId": "lane_candidate_01"
      },
      "candidateValue": {
        "centerline": [[120.0, 40.0, 0.0], [160.0, 55.0, 0.0]],
        "width": 3.5
      },
      "topologySuggestions": [
        {
          "relation": "predecessor",
          "objectId": "lane_main_02"
        }
      ],
      "confidence": 0.87
    }
  ],
  "status": "received"
}
```

## 5. ChangeSet

### 5.1 定义

`ChangeSet` 是平台内部经过审核、必要修订后形成的正式变更集合，是从一个地图版本演进到下一个地图版本的唯一正式输入。

首版地图也使用 ChangeSet：

```text
空地图
→ initial ChangeSet
→ MapVersion V1
```

### 5.2 数据方向

```text
CandidateMapPatch / 人工编辑 / 初始导入
                    ↓
              平台审核与修订
                    ↓
                 ChangeSet
                    ↓
                MapVersion
```

### 5.3 字段

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `schemaVersion` | string | 是 | ChangeSet 契约版本 |
| `changeSetId` | string | 是 | 全局唯一变更集合 ID |
| `mapId` | string | 是 | 所属地图项目 |
| `baseMapVersion` | string/null | 是 | 父版本；首版 initial ChangeSet 为 `null` |
| `type` | enum | 是 | `initial`、`manual_edit`、`candidate_patch` 或 `mixed` |
| `sourcePatchIds` | array | 否 | 关联的 Candidate Patch ID |
| `title` | string | 是 | 简短变更标题 |
| `reason` | string | 是 | 变更原因 |
| `operations` | array | 是 | 审核后正式操作集合 |
| `status` | enum | 是 | 当前生命周期状态 |
| `validationReportId` | string | 否 | 最近一次质检报告 ID |
| `createdAt` | datetime | 是 | 创建时间 |
| `createdBy` | string | 是 | 创建者 |
| `approvedAt` | datetime | 否 | 审核通过时间 |
| `approvedBy` | string | 否 | 审核者 |
| `resultingMapVersion` | string | 否 | 提交后生成的新地图版本 |

正式 `operations` 元素包含：

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `operationId` | string | 是 | ChangeSet 内唯一 ID |
| `action` | enum | 是 | `create`、`update` 或 `delete` |
| `target` | ObjectReference | 是 | 目标地图对象 |
| `before` | object/null | 是 | 变更前完整对象；创建操作为 `null` |
| `after` | object/null | 是 | 变更后完整对象；删除操作为 `null` |
| `sourcePatchIds` | array | 否 | 当前操作来源的候选 Patch |

保存完整 `before` 和 `after` 可以让首版实现更容易完成差异展示、审计和回滚。以后若数据规模增大，可在保持语义不变的前提下优化存储。

### 5.4 状态机

```text
draft
  ↓
validating
  ├─► rejected
  └─► ready
         ↓
      committed
```

| 状态 | 含义 |
|---|---|
| `draft` | 仍允许平台编辑 |
| `validating` | 正在执行几何、拓扑和业务规则质检 |
| `ready` | 质检通过，等待提交为新地图版本 |
| `committed` | 已生成不可变 MapVersion，ChangeSet 不再修改 |
| `rejected` | 未通过审核或决定放弃 |

### 5.5 关键约束

- 一个已提交 ChangeSet 只能生成一个 `MapVersion`；
- 非首版 ChangeSet 的 `baseMapVersion` 必须存在且与创建工作草稿时的基线一致；首版 initial ChangeSet 的该字段必须为 `null`；
- `before` 必须与基准版本中的对象一致，否则视为版本冲突；
- Fatal 或 Error 级质检问题未清零时不能进入 `ready`；
- `committed` 后禁止修改操作内容。

## 6. MapVersion

### 6.1 定义

`MapVersion` 是经过审核和质检后冻结的完整逻辑地图快照，是平台侧正式地图的 Source of Truth。

它不是车端二进制地图包，也不是可继续编辑的工作草稿。

### 6.2 数据方向

```text
ChangeSet
    ↓
平台版本发布
    ↓
MapVersion
    ├─► 地图差异、审计和回滚基线
    └─► 地图编译器
```

### 6.3 字段

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `schemaVersion` | string | 是 | MapVersion 契约版本 |
| `mapId` | string | 是 | 地图项目 ID |
| `version` | string | 是 | 地图版本号，例如 `V1` |
| `parentVersion` | string/null | 是 | 父版本；V1 为 `null` |
| `sourceChangeSetId` | string | 是 | 产生该版本的 ChangeSet |
| `status` | enum | 是 | 版本状态 |
| `description` | string | 是 | 版本说明 |
| `mapSchemaVersion` | string | 是 | Canonical Map Model 版本 |
| `coordinateReference` | object | 是 | 坐标系、单位和原点引用 |
| `objectCounts` | object | 是 | 各类地图对象数量 |
| `validationSummary` | object | 是 | 发布质检摘要和报告 ID |
| `contentHash` | string | 是 | 规范化地图内容的 SHA-256 |
| `createdAt` | datetime | 是 | 版本创建时间 |
| `createdBy` | string | 是 | 版本创建者 |
| `publishedAt` | datetime | 否 | 正式发布时间 |
| `publishedBy` | string | 否 | 发布者 |

逻辑地图内容本身可以保存在版本目录中的 `map.json`，也可以存储在数据库快照中，但必须能够仅凭版本记录恢复完全一致的地图内容。

### 6.4 状态机

```text
building
   ↓
validating
   ├─► failed
   └─► ready
          ↓
       published
          ↓
       withdrawn
```

| 状态 | 含义 |
|---|---|
| `building` | 正在根据 ChangeSet 生成版本快照 |
| `validating` | 正在执行发布前最终校验 |
| `ready` | 内容已冻结并通过校验，等待正式发布 |
| `published` | 正式可编译、可作为 Patch 基准的版本 |
| `failed` | 版本构建或验证失败，不得发布 |
| `withdrawn` | 已停止继续分发，但历史记录仍然保留 |

### 6.5 不可变约束

- `ready` 后地图内容已经冻结；
- `published` 后任何内容修改必须生成子版本；
- `contentHash` 相同的版本内容必须完全一致；
- `withdrawn` 不能删除版本文件和审计记录；
- V2 的 `parentVersion` 必须明确指向 V1，不能只靠版本号推测关系。

### 6.6 最小目录结构

```text
maps/versions/logistics_park/V1/
├─ manifest.json
├─ map.json
├─ validation-report.json
└─ initial-changeset.json
```

## 7. MapPackage

### 7.1 定义

`MapPackage` 是地图编译器从一个已发布 `MapVersion` 生成的不可编辑车端运行制品。

同一个 MapVersion 可以针对不同处理器、SDK ABI 或运行时格式生成多个 MapPackage，但每个包只能来源于一个确定的 MapVersion。

### 7.2 数据方向

```text
已发布 MapVersion
        ↓
    C++ 编译器
        ↓
    MapPackage
        ↓
发布服务 / HTTP 下载
        ↓
   Vehicle Map SDK
```

### 7.3 字段

| 字段 | 类型 | 必填 | 含义 |
|---|---|---:|---|
| `schemaVersion` | string | 是 | Package Manifest 契约版本 |
| `packageId` | string | 是 | 地图包唯一 ID |
| `mapId` | string | 是 | 地图项目 ID |
| `mapVersion` | string | 是 | 唯一来源 MapVersion |
| `mapContentHash` | string | 是 | 来源 MapVersion 的内容 Hash |
| `packageFormat` | string | 是 | 运行时格式名称，例如 `automap_runtime` |
| `formatVersion` | string | 是 | 二进制格式版本 |
| `compilerVersion` | string | 是 | 编译器版本 |
| `target` | object | 是 | 目标 OS、架构、字节序及 SDK ABI |
| `coordinateReference` | object | 是 | 与来源地图一致的坐标参考 |
| `statistics` | object | 是 | Lane、Junction、几何点和拓扑边数量 |
| `files` | array | 是 | 文件名、大小、用途和 SHA-256 |
| `status` | enum | 是 | 地图包状态 |
| `createdAt` | datetime | 是 | 编译完成时间 |
| `publishedAt` | datetime | 否 | 开始对车端发布的时间 |

首版 `files` 固定为：

```text
manifest.json
map.bin
spatial.index
```

### 7.4 状态机

```text
building
   ↓
verifying
   ├─► failed
   └─► ready
          ↓
       published
          ├─► deprecated
          └─► revoked
```

| 状态 | 含义 |
|---|---|
| `building` | 编译器正在生成文件 |
| `verifying` | 正在反向加载并校验 Hash、偏移、数量和拓扑 |
| `ready` | 包验证通过，尚未对车辆发布 |
| `published` | 可供兼容车端下载和切换 |
| `deprecated` | 仍可用于回滚，但不再推荐新车辆使用 |
| `revoked` | 存在安全或数据问题，不得继续激活 |
| `failed` | 编译或验证失败，不得发布 |

### 7.5 关键约束

- 只有 `published` 的 MapVersion 才能产生正式 MapPackage；
- 包必须保存来源 `mapContentHash`，防止版本号相同但内容不一致；
- 所有二进制文件都必须记录大小和 SHA-256；
- 地图包不可被车端编辑；
- 车端必须先验证再激活，验证失败时继续使用旧包；
- `revoked` 与 `deprecated` 不删除历史制品，便于审计。

## 8. DynamicEvent

### 8.1 定义

`DynamicEvent` 是平台下发给车端的临时运行时覆盖信息，用于施工封路、事故、临时限速等短期变化。

它不属于 MapVersion，不修改 MapPackage，也不能承载永久几何和拓扑变更。

### 8.2 数据方向

```text
调度系统 / 平台操作员
          ↓
      DynamicEvent
          ↓
        车端
          ↓
基础 MapPackage + 事件覆盖层
```

### 8.3 字段

| 字段 | 类型 | 必填 | 写入方 | 含义 |
|---|---|---:|---|---|
| `schemaVersion` | string | 是 | 平台 | Dynamic Event 契约版本 |
| `eventId` | string | 是 | 平台 | 稳定事件 ID |
| `revision` | integer | 是 | 平台 | 同一事件的递增修订号 |
| `mapId` | string | 是 | 平台 | 目标地图项目 |
| `compatibleVersions` | object | 是 | 平台 | 可应用的地图版本范围或列表 |
| `eventType` | enum | 是 | 平台 | 事件类型 |
| `targets` | array | 是 | 平台 | 受影响地图对象 |
| `effect` | object | 是 | 平台 | 对目标对象的运行时覆盖内容 |
| `priority` | integer | 是 | 平台 | 多事件冲突时的优先级 |
| `effectiveFrom` | datetime | 是 | 平台 | 开始生效时间 |
| `effectiveUntil` | datetime | 是 | 平台 | 失效时间 |
| `status` | enum | 是 | 平台 | 平台侧事件状态 |
| `reason` | string | 是 | 平台 | 事件原因 |
| `issuedAt` | datetime | 是 | 平台 | 当前修订发布时间 |
| `issuedBy` | string | 是 | 平台 | 发布者 |

首版 `eventType`：

```text
lane_closed
temporary_speed_limit
restricted_access
```

`effect` 示例：

```json
{
  "access": "closed"
}
```

或：

```json
{
  "speedLimit": 5.0,
  "unit": "m/s"
}
```

### 8.4 状态机

```text
scheduled
    ↓ 到达 effectiveFrom
 active
   ├─► cancelled
   └─► expired
```

| 状态 | 含义 |
|---|---|
| `scheduled` | 已发布但尚未到生效时间 |
| `active` | 当前应由车端应用 |
| `cancelled` | 平台提前撤销；通过更高 revision 通知车端 |
| `expired` | 已超过失效时间，不再应用 |

车端不能只信任消息中的 `status`，还必须依据本地可信时间检查 `effectiveFrom` 和 `effectiveUntil`。

### 8.5 关键约束

- `revision` 必须单调递增，车端忽略旧修订；
- 事件只能作用于当前地图包存在的对象；
- 多事件冲突时先比较 `priority`，再比较 `revision`；
- 永久新增道路、删除道路和修改拓扑必须走 ChangeSet 与 MapVersion；
- 事件过期、撤销或不兼容时必须移除覆盖并恢复基础地图状态；
- Dynamic Event 只改变运行时可用性，不改变 MapVersion 的内容 Hash。

### 8.6 最小示例

```json
{
  "schemaVersion": "1.0",
  "eventId": "event_lane_close_001",
  "revision": 1,
  "mapId": "logistics_park_demo",
  "compatibleVersions": {
    "versions": ["V1", "V2"]
  },
  "eventType": "lane_closed",
  "targets": [
    {
      "objectType": "lane",
      "objectId": "lane_main_02"
    }
  ],
  "effect": {
    "access": "closed"
  },
  "priority": 100,
  "effectiveFrom": "2026-08-06T14:00:00+08:00",
  "effectiveUntil": "2026-08-06T18:00:00+08:00",
  "status": "scheduled",
  "reason": "temporary_construction",
  "issuedAt": "2026-08-06T13:50:00+08:00",
  "issuedBy": "operator_01"
}
```

## 9. 五类对象关系

| 来源对象 | 目标对象 | 关系 | 基数 |
|---|---|---|---|
| CandidateMapPatch | MapVersion | `baseMapVersion` 指向观测时基准版本 | 多对一 |
| CandidateMapPatch | ChangeSet | 接受的 Patch 可成为 ChangeSet 来源 | 多对一或多对多 |
| ChangeSet | MapVersion | 一个已提交 ChangeSet 生成一个新版本 | 一对一 |
| MapVersion | MapVersion | 通过 `parentVersion` 构成版本链 | 多对一 |
| MapVersion | MapPackage | 一个版本可针对不同目标生成多个包 | 一对多 |
| DynamicEvent | MapVersion | 只声明兼容版本，不改变版本内容 | 多对多 |
| DynamicEvent | MapPackage | 由车端在兼容包上建立运行时覆盖 | 多对多 |

完整关系示例：

```text
MapVersion V1
    ├─ MapPackage V1 / windows-x64
    ├─ MapPackage V1 / vehicle-linux-arm64
    └─ Candidate Patch P1、P2 的 baseMapVersion

Candidate Patch P1 + P2
    ↓ 审核、融合、修订
ChangeSet CS2
    ↓ 提交
MapVersion V2（parent = V1）
    ↓ 编译
MapPackage V2

Dynamic Event E1
    └─ compatibleVersions = [V1, V2]
```

## 10. 跨对象一致性规则

首版至少执行以下契约级检查：

1. 所有 ID 在各自命名空间内唯一；
2. Candidate Patch 的 `baseMapVersion` 必须存在；
3. 接受 Patch 前必须检查它与当前工作基线是否冲突；
4. ChangeSet 中的 `before` 必须匹配基准版本；
5. ChangeSet 通过质检后才能提交；
6. MapVersion 的 `sourceChangeSetId`、`parentVersion` 必须可追溯；
7. MapVersion 发布后内容和 `contentHash` 不可改变；
8. MapPackage 的 `mapContentHash` 必须等于来源 MapVersion；
9. MapPackage 发布前必须完成反向加载验证；
10. Dynamic Event 的目标对象必须存在于车端当前兼容地图包；
11. Dynamic Event 不得新增永久几何或永久拓扑；
12. 时间、版本或 Hash 不兼容时，车端必须拒绝应用数据，而不是猜测处理。

## 11. 首版实现边界

MVP 对这些契约采用以下简化：

- Candidate Patch 由模拟器生成，不实现真实车端感知和 SLAM；
- ChangeSet 保存完整 `before` 和 `after`，暂不优化为字段级二进制差分；
- MapVersion 第一阶段使用目录快照，后续再接入数据库版本存储；
- MapPackage 第一阶段只面向一个本地测试目标；
- Dynamic Event 第一阶段通过本地文件或 HTTP 拉取模拟，不实现量产 OTA 和消息中间件；
- 所有状态变化先保证可追溯和可测试，不实现复杂多人审批。

## 12. 后续落地文件

本文通过评审后，后续任务应分别生成：

```text
schemas/candidate-map-patch.schema.json
schemas/change-set.schema.json
schemas/map-version.schema.json
schemas/map-package-manifest.schema.json
schemas/dynamic-event.schema.json
```

C++ 类型应放在对应领域模块中，JSON Schema 和 C++ 校验必须表达相同的必填字段、枚举和不变量。
