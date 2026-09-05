<script setup lang="ts">
import type { SelectedObject } from "../model";
import { ref, watch } from "vue";

const props = defineProps<{ selected?: SelectedObject }>();
const emit = defineEmits<{ close: [] }>();
const copyStatus = ref("");
watch(() => props.selected?.key, () => { copyStatus.value = ""; });
async function copy(): Promise<void> {
  try { await navigator.clipboard.writeText(JSON.stringify(props.selected?.value, null, 2)); copyStatus.value = "已复制"; }
  catch { copyStatus.value = "复制失败，请选中文本手动复制"; }
}
</script>

<template>
  <aside class="panel inspector-panel">
    <div class="candidate-heading"><p class="section-label">原始数据</p><button class="icon-button" aria-label="收起原始数据面板" @click="emit('close')">×</button></div>
    <template v-if="selected">
      <h2>{{ selected.kind }} · {{ selected.id }}</h2>
      <p class="muted">内部稳定键：{{ selected.key }}</p>
      <button class="attribute-link" @click="copy">复制 JSON</button><span role="status" class="muted">{{ copyStatus }}</span>
      <pre id="object-json">{{ JSON.stringify(selected.value, null, 2) }}</pre>
    </template>
    <template v-else>
      <h2>尚未选择对象</h2>
      <p class="muted">点击地图对象，或使用左侧对象选择器搜索。</p>
      <pre id="object-json">{}</pre>
    </template>
  </aside>
</template>
