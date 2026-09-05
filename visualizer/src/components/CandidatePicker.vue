<script setup lang="ts">
import type { RenderFeature } from "../render/types";

defineProps<{ candidates: RenderFeature[] }>();
const emit = defineEmits<{ select: [key: string]; close: [] }>();
</script>

<template>
  <div v-if="candidates.length > 1" class="candidate-picker" role="dialog" aria-label="选择重叠对象">
    <div class="candidate-heading">
      <strong>此处有 {{ candidates.length }} 个对象</strong>
      <button type="button" class="icon-button" @click="emit('close')">×</button>
    </div>
    <button v-for="feature in candidates" :key="feature.key" type="button" @click="emit('select', feature.key)">
      {{ feature.kind }} · {{ feature.name || feature.id }}
    </button>
  </div>
</template>
