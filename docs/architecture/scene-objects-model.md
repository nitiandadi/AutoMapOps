# 场景对象模型

场景对象把“道路能否行驶”之外的园区业务语义挂到 Canonical 地图上。它们不是 OpenDRIVE 的标准对象集合，而是平台内部的教学型业务模型。

## OperationalArea

`OperationalArea` 表示有范围的业务区域，包含稳定 ID、名称、类型和闭合 ENU 折线 `outline`。首版类型包括仓库、装卸区、停车区和充电区。首尾点是否闭合、面积是否有效由 M3 几何质检判断。

## Station

`Station` 表示车辆可以规划到达的具体业务目标点，例如门岗、月台、停车位、充电点或普通途经点。`position` 是局部 ENU 坐标，`access_lane_id` 指向实际用于到达该站点的 Lane。Station 本身不是铁路站点，也不替代 Lane 几何。

## RestrictedArea

`RestrictedArea` 表示具有准入规则的闭合区域。`allowed_vehicle_profile_ids` 是白名单：空列表的具体业务含义暂由质检/应用规则确定，不在值类型中武断解释为“全部允许”或“全部禁止”。

## OpenDRIVE 导出

根据用途，这些对象可选择映射到 OpenDRIVE 的 `<object>`、停车空间、用户自定义数据或外围业务文件。OpenDRIVE 主要描述道路与车道，不能保证无损表达园区仓库、充电任务和车辆白名单，所以导出器必须记录降级或扩展策略。
