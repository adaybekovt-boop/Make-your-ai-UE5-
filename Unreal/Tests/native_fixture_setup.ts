import { spawnSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import path from 'node:path'

export default function setup() {
  const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..')
  const python = process.env.PYTHON ?? (process.platform === 'win32' ? 'python' : 'python3')
  const result = spawnSync(python, ['Unreal/Tests/native_runner.py', 'core'], {
    cwd: root, stdio: 'inherit', timeout: 420_000,
  })
  if (result.error) throw result.error
  if (result.status !== 0) throw new Error(`Native fixture generation failed: exit=${result.status}, signal=${result.signal}`)
}
