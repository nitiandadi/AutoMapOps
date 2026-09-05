<script setup lang="ts">
import { computed, ref, watch } from "vue";
import type { RenderFeature } from "../render/types";

const props = defineProps<{ results: RenderFeature[]; query: string; selectedKey: string }>();
const emit = defineEmits<{ "update:query": [value: string]; select: [key: string] }>();
const page = ref(0);
const pageSize = 50;
const pageCount = computed(() => Math.max(1, Math.ceil(props.results.length / pageSize)));
const visibleResults = computed(() => props.results.slice(page.value * pageSize, (page.value + 1) * pageSize));
watch(() => [props.query, props.results.length], () => { page.value = 0; });
</script>

<template>
  <section class="object-browser">
    <p class="section-label">对象选择器</p>
    <input
      class="search-input"
      type="search"
      :value="query"
      placeholder="搜索 ID、名称或类型"
      @input="emit('update:query', ($event.target as HTMLInputElement).value)"
    />
    <p class="muted result-count">{{ results.length }} 个对象 · 第 {{ page + 1 }}/{{ pageCount }} 页</p>
    <div class="object-results">
      <button
        v-for="feature in visibleResults"
        :key="feature.key"
        type="button"
        class="object-result"
        :class="{ active: feature.key === selectedKey }"
        @click="emit('select', feature.key)"
      >
        <span>{{ feature.name || feature.id }}</span>
        <small>{{ feature.kind }} · {{ feature.id }}</small>
      </button>
    </div>
    <div v-if="pageCount > 1" class="pagination">
      <button type="button" :disabled="page === 0" @click="page -= 1">上一页</button>
      <button type="button" :disabled="page + 1 >= pageCount" @click="page += 1">下一页</button>
    </div>
  </section>
</template>
