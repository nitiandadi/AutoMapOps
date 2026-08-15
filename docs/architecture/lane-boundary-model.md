# LaneBoundary 车道边界模型

`LaneBoundary` 描述车道两侧可观察或逻辑存在的边界。它是可复用的独立地图对象，通过稳定 ID 被 `Lane.left_boundary_id` 和 `Lane.right_boundary_id` 引用。

## 字段

| C++ 字段 | Canonical JSON 名称 | 含义 |
|---|---|---|
| `id` | `id` | 地图内稳定且唯一的业务 ID |
| `geometry` | `geometry` | 局部 ENU 坐标中的三维折线，单位为米 |
| `type` | `type` | 边界的物理或逻辑类型 |
| `crossing_allowed` | `crossingAllowed` | 当前静态地图规则是否允许车辆跨越 |

首版边界类型为：

- `unknown`：类型尚未确定，常用于未完成的草稿；
- `dashed_line`：虚线；
- `solid_line`：单实线；
- `double_solid_line`：双实线；
- `curb`：路缘石等物理边界；
- `virtual_boundary`：没有实体标线、但业务上需要约束车道范围的逻辑边界。

## 几何方向约定

边界折线的点序统一沿所属 `Road.reference_line` 的正方向排列，不能因为某条 Lane 的实际行驶方向相反而翻转。这样一条共享边界只有一份稳定几何。

`Lane.left_boundary_id` 和 `Lane.right_boundary_id` 则以该 Lane 的实际行驶方向观察左右。对于 `against_reference_line` 的 Lane，读取者不能把数组点序直接理解成车辆行驶方向。

## 共享边界

相邻车道可以引用同一个 `LaneBoundary`。例如双向道路的中心虚线既可以是正向车道的左边界，也可以是反向车道的右边界。共享对象避免重复几何产生偏差，并便于后续质检相邻关系。

## 类型与跨越权限分离

`type` 回答“边界长什么样”，`crossing_allowed` 回答“静态规则是否允许跨越”。两者相关但不等价，因此模型不根据类型自动推导权限：园区内可能存在允许特定作业车辆跨越的实线，也可能存在因区域规则而禁止跨越的虚线。后续质检可以对常识上矛盾的组合给出警告，但不在核心模型中拒绝草稿数据。

## 与 OpenDRIVE 的关系

本模型不是 OpenDRIVE 字段的原样复制。导出时可把 `dashed_line`、`solid_line`、`double_solid_line` 近似映射到 `<roadMark type="broken|solid|solid solid">`；`curb` 和 `virtual_boundary` 需要结合目标工具支持情况选择 roadMark、lane 类型或自定义扩展。`crossing_allowed` 也需要依据目标 OpenDRIVE 版本及应用约定转换，不能仅靠标线类型无损表达。

## 首版限制

M2-04 暂不表达颜色、线宽、材质、标线分段、左右组合线、时间条件和车辆分类权限。这些属性可在真实导入导出需求明确后扩展，当前模型足以支持物流园 V0 的基础边界、共享关系和静态跨越规则。
