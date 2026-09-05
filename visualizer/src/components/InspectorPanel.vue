<script setup lang="ts">
import type { SelectedObject } from "../model";

defineProps<{ selected?: SelectedObject }>();
</script>

<template>
  <aside class="panel inspector-panel">
    <p class="section-label">对象检查器</p>
    <template v-if="selected">
      <h2>{{ selected.kind }} · {{ selected.id }}</h2>
      <p class="muted">内部稳定键：{{ selected.key }}</p>
      <pre id="object-json">{{ JSON.stringify(selected.value, null, 2) }}</pre>
    </template>
    <template v-else>
      <h2>尚未选择对象</h2>
      <p class="muted">点击地图对象，或使用左侧对象选择器搜索。</p>
      <pre id="object-json">{}</pre>
    </template>
  </aside>
</template>
