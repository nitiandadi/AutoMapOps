<script setup lang="ts">
import type { FeatureKind } from "../model";
import type { RenderFeature } from "../render/types";

const props = defineProps<{ value: Record<string, unknown>; kind: FeatureKind; features: RenderFeature[] }>();
const emit = defineEmits<{ select: [key: string] }>();
const names: Record<string, string> = {
  id: "对象 ID", name: "名称", type: "类型", roadId: "所属道路", laneIds: "所属车道",
  predecessorIds: "前驱", successorIds: "后继", leftBoundaryId: "左边界", rightBoundaryId: "右边界",
  side: "参考线侧别", orderFromReference: "距参考线序号", direction: "行驶方向", status: "通行状态",
  widthM: "宽度（m）", heightM: "高度（m）", lengthM: "长度（m）", speedLimitMps: "限速（m/s）",
  minimumTurningRadiusM: "最小转弯半径（m）", crossingAllowed: "允许跨越", connectionIds: "连接关系",
  junctionId: "所属路口", incomingLaneId: "入口车道", connectingLaneId: "连接车道", outgoingLaneId: "出口车道",
  turnDirection: "转向", accessLaneId: "接入车道", allowedVehicleProfileIds: "允许车辆",
  headingRad: "起始航向（rad）", endZM: "终点高程（m）", curvaturePerM: "曲率（1/m）",
  startCurvaturePerM: "起始曲率（1/m）", endCurvaturePerM: "终点曲率（1/m）", start: "起点 ENU（m）",
  position: "位置 ENU（m）", outline: "区域轮廓", from: "起始车道", to: "目标车道",
};
const enums: Record<string, string> = {
  solid_line: "实线", dashed_line: "虚线", double_solid_line: "双实线", curb: "路缘", virtual_boundary: "虚拟边界",
  along_reference_line: "沿参考线", against_reference_line: "逆参考线", open: "开放", closed: "关闭",
  left: "左", right: "右", straight: "直行", u_turn: "掉头", unknown: "未知",
  warehouse: "仓库", loading_area: "装卸区", parking_area: "停车区", charging_area: "充电区",
  gate: "门岗", loading_bay: "装卸月台", parking_space: "停车位", charging_point: "充电点", waypoint: "途经点",
  passenger_car: "乘用车", delivery_van: "配送车", truck: "货车", line: "直线", circular_arc: "圆弧", clothoid: "缓和曲线",
};
function format(value: unknown, field: string): string {
  if (value == null || value === "") return "未设置";
  if (typeof value === "boolean") return value ? "是" : "否";
  if (typeof value === "number") return Number.isInteger(value) ? String(value) : value.toFixed(6).replace(/0+$/, "").replace(/\.$/, "");
  if (Array.isArray(value)) return value.length ? value.map(v => format(v, field)).join("，") : "未设置";
  return field === "id" || field === "name" ? String(value) : enums[String(value)] ?? String(value);
}
function links(field: string, value: unknown): RenderFeature[] {
  const kinds: Record<string, FeatureKind> = {
    roadId: "Road", laneIds: "Lane", leftBoundaryId: "LaneBoundary", rightBoundaryId: "LaneBoundary",
    connectionIds: "LaneConnection", junctionId: "Junction", incomingLaneId: "Lane", connectingLaneId: "Lane",
    outgoingLaneId: "Lane", accessLaneId: "Lane", allowedVehicleProfileIds: "VehicleProfile", from: "Lane", to: "Lane",
    predecessorIds: props.kind, successorIds: props.kind,
  };
  const ids = Array.isArray(value) ? value : [value];
  return kinds[field] ? props.features.filter(f => f.kind === kinds[field] && ids.includes(f.id)) : [];
}
</script>

<template>
  <table class="attribute-table"><tbody>
    <tr v-for="(item, field) in value" :key="field">
      <th scope="row">{{ names[field] || field }}</th>
      <td>
        <template v-if="links(field, item).length">
          <button v-for="target in links(field, item)" :key="target.key" class="attribute-link" @click="emit('select', target.key)">{{ target.id }}</button>
        </template>
        <details v-else-if="item && typeof item === 'object' && !Array.isArray(item)">
          <summary>展开属性</summary>
          <AttributeTable :value="item as Record<string, unknown>" :kind="kind" :features="features" @select="emit('select', $event)" />
        </details>
        <details v-else-if="Array.isArray(item) && item.some(v => typeof v === 'object')">
          <summary>{{ item.length }} 项</summary>
          <div v-for="(entry, index) in item" :key="index">{{ index + 1 }} · {{ format(entry, field) }}</div>
        </details>
        <template v-else>{{ format(item, field) }}</template>
      </td>
    </tr>
  </tbody></table>
</template>
