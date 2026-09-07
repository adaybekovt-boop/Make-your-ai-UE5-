# UE5 — статус текущего source-level дополнения

**SOURCE_SCAFFOLD. UE5 reference gate: NOT VERIFIED.**

Ветка: `agent/ue5-core-vertical-slice`. Проверенный первоначальный пакет этого дополнения: `c9c3e0e472a07c3adb2ec41416455747cc102eb9`. Проверенный объединённый код после параллельного обновления: `b1fe36bb67ffc1fb08e0ab8cf6d66586c559b19e`. Финальный отчёт и исправление времени жизни определений Garage добавляются поверх этого HEAD, не вместо чужих коммитов. SHA собственного отчётного коммита: `git log -1 --format=%H -- Unreal/UE5_STATUS.md`.

## Реально доступная среда

Редактирование: Linux x86_64, GCC 14.2.0; Node 22.16.0 и npm 11.4.2. `clang++`, UnrealEditor, UnrealEditor-Cmd, UnrealBuildTool, nvidia-smi и Blender не найдены в PATH. UE_ROOT не задан. `/dev/dri` отсутствует; DISPLAY/Wayland не заданы. GPU/Editor rendering session недоступны. Установленная версия UE5 неизвестна — она не подставлялась по памяти.

Локальная папка является распакованным снимком исходников, **не Git clone**. Чтение/публикация выполнялись GitHub connector; настоящие checkout и браузерные npm-проверки — GitHub Actions. Локальный GCC компилировал переносимые production C++-файлы кампании, а не mock-замену и не UE-заголовки. Исторические сведения о другой машине в старых коммитах не являются текущей средой.

CI на `b1fe36b`: Ubuntu 24.04.4, GCC 13.3.0, Node 22.23.2, npm 10.9.8. Проверены job `101833955477`, run `34151292202`, полный лог и скачанный native evidence artifact `10029461032`. [Журнал](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34151292202/job/101833955477).

## Проверки по категориям

- Native domain C++: собран с ASan/UBSan; 46 сценариев / 1 355 assertions, код 0.
- Отдельная кампания: 43 сценария / 1 549 assertions, код 0; реальный restart в новом нативном процессе совпал с непрерывным прогоном.
- Browser: npm ci, 388 тестов, typecheck и build — код 0.
- C++/TypeScript parity: 347 тестов — код 0.
- Python: 16 preparation + 23 source/runner checks — код 0.
- UE compile: NOT VERIFIED; UHT/UBT не запускались.
- Unreal Automation: NOT VERIFIED; написанные тесты не равны исполненным.
- Editor maps / collisions / visual / full clicked gameplay: NOT VERIFIED.

Локальная команда `scaffold_runner.py build-campaign` возвращает wrapper exit 2 без UE_ROOT. `process_exit_code` остаётся null: это отказ до запуска Editor, не неудачная компиляция UE. Проверки source-hashes подтверждают байты файлов, а не Unreal API-совместимость.

## Сохраняющиеся ограничения

Бинарные UE-карты этим прогоном не создавались, исходные FBX/GLB/Blend не заменялись, main не мержился. Связанная ходьба Garage — C++ graybox; анимация персонажа и художественная интеграция интерьерного FBX не подтверждены. Nanite/Lumen/World Partition/LOD/FPS — отдельный непроверенный этап.

Параллельное обновление переименовало сложности в Easy / Normal / Hard и расширило payload/экраны; оно сохранено. Это расхождение с именами Startup / Standard / Hardcore из данного задания явно открыто, как и проверка совместной работы обоих UI-путей в Editor. Новая review/ending-экономика не прошла длительную живую партию 1×. Доход всего браузерного портфеля и все контракты полностью не перенесены.

Следующий блокер: установленный и доступный UE5 с подходящим toolchain; сначала UHT/Development Editor, затем настоящие загрузки, ходьба/trace, SaveGame и UMG-прохождение. Зелёный native CI этого не заменяет.
