import { defineConfig } from 'vitest/config'

export default defineConfig({
  test: {
    include: ['Unreal/Tests/*.parity.ts'],
    environment: 'node',
    globalSetup: ['Unreal/Tests/native_fixture_setup.ts'],
    fileParallelism: false,
  },
})
