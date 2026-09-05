<script setup lang="ts">
import type { LayerName } from "../model";

defineProps<{ visibility: Record<LayerName, boolean> }>();
const emit = defineEmits<{ toggle: [layer: LayerName, visible: boolean] }>();

const layers: Array<{ key: LayerName; label: string }> = [
  { key: "areas", label: "业务区域" }, { key: "restricted", label: "限制区域" },
  { key: "roads", label: "Road 参考线" }, { key: "boundaries", label: "LaneBoundary" },
  { key: "lanes", label: "Lane 中心线" }, { key: "topology", label: "拓扑连接" },
  { key: "junctions", label: "路口对象" }, { key: "stations", label: "Station" },
  { key: "labels", label: "对象标签" },
];
</script>

<template>
  <section>
    <p class="section-label">图层</p>
    <div class="layer-list">
      <label v-for="layer in layers" :key="layer.key" class="layer-toggle">
        <input
          type="checkbox"
          :checked="visibility[layer.key]"
          @change="emit('toggle', layer.key, ($event.target as HTMLInputElement).checked)"
        />
        <span>{{ layer.label }}</span>
      </label>
    </div>
  </section>
</template>
