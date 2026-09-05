<script setup lang="ts">
import { computed } from "vue";
import type { PathGeometry3d } from "../model";
import { isCompositeCurve } from "../model";
import { pathLength } from "../render/geometry";
import type { RenderFeature } from "../render/types";
import AttributeTable from "./AttributeTable.vue";

const props = defineProps<{ selected: RenderFeature; features: RenderFeature[] }>();
const emit = defineEmits<{ close: []; select: [key: string]; json: [] }>();
const properties = computed(() => Object.fromEntries(Object.entries(props.selected.value)
  .filter(([key]) => !["referenceLine", "centerline", "geometry"].includes(key))));
const geometry = computed(() => props.selected.geometry as PathGeometry3d | undefined);
</script>

<template>
  <section class="property-popup" role="dialog" aria-label="对象属性" @pointerdown.stop @click.stop @wheel.stop @dblclick.stop @keydown.esc="emit('close')">
    <header class="popup-heading">
      <div><small>{{ selected.kind }}</small><h2>{{ selected.name || selected.id }}</h2></div>
      <button class="icon-button" aria-label="关闭属性弹窗" @click="emit('close')">×</button>
    </header>
    <div class="popup-body">
      <AttributeTable :value="properties" :kind="selected.kind" :features="features" @select="emit('select', $event)" />
      <template v-if="geometry">
        <h3>路径几何</h3>
        <AttributeTable :value="{ '表示类型': isCompositeCurve(geometry) ? '组合曲线' : '点列', '平面总长度（m）': pathLength(geometry), '段数 / 点数': isCompositeCurve(geometry) ? geometry.segments.length : geometry.length }" :kind="selected.kind" :features="features" />
        <details v-if="isCompositeCurve(geometry)"><summary>曲线段参数（{{ geometry.segments.length }}）</summary>
          <details v-for="(segment, index) in geometry.segments" :key="index" class="segment-details">
            <summary>第 {{ index + 1 }} 段</summary>
            <AttributeTable :value="{ ...segment }" :kind="selected.kind" :features="features" />
          </details>
        </details>
      </template>
    </div>
    <footer><button class="attribute-link" @click="emit('json')">查看原始 JSON</button></footer>
  </section>
</template>
