import { defineConfig } from 'vitest/config'

export default defineConfig({
  test: {
    include: ['Unreal/Tests/*.parity.ts'],
    environment: 'node',
    fileParallelism: false,
  },
})
