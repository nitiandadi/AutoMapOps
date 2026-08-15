# MapData 聚合根

`MapData` 是一张 Canonical 逻辑地图的聚合根，按值持有 `MapHeader` 和九类地图对象：Road、Lane、LaneBoundary、Junction、LaneConnection、OperationalArea、Station、RestrictedArea、VehicleProfile。

所有对象容器使用 `std::vector`，以保持 JSON 顺序并允许草稿暂时包含重复 ID或悬空引用。对象之间只通过稳定字符串 ID 关联，不保存跨容器指针。

`find_*` 为每类对象提供可变和只读查询，未找到时统一返回 `nullptr`。它们是线性查找，适合生产模型和小型教学地图；M4 编译为车端地图包时再建立紧凑 ID 与专用索引。

`MapData` 及其全部子对象支持值比较，为 M3 Canonical JSON 的“写出—读回—比较”测试提供基础。值比较要求容器顺序一致；ID 唯一性、引用完整性、拓扑互反和业务合法性仍由 `map_validation` 负责。
