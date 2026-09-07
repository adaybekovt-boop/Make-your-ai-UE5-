# Среда и статус UE5 — 7 сентября 2026

SOURCE_SCAFFOLD. UE COMPILE_VERIFIED / EDITOR_VERIFIED / VISUAL_VERIFIED / GAMEPLAY_VERIFIED: NOT VERIFIED.

## Проверенная среда редактирования

Linux x86_64, glibc 2.36. В PATH найдены GCC 12.2.0 (`/usr/bin/g++`), Clang 14.0.6 (`/usr/bin/clang++`), Node 22.16.0, npm 10.9.2. Все четыре команды версии завершились кодом 0. Наличие компилятора не означает установленный UE toolchain.

UE_ROOT не установлен. UnrealEditor, UnrealEditor-Cmd, UnrealBuildTool, Blender и nvidia-smi не найдены в PATH. В проверенных стандартных каталогах кандидаты Engine/Build/Build.version не найдены. `/dev/dri` отсутствует. DISPLAY задан, WAYLAND_DISPLAY отсутствует: сам DISPLAY не подтверждает наличие GPU или рабочего сеанса Unreal Editor. Сеанс UE с графическим выводом не запускался.

Локальная рабочая папка не является Git clone. Чтение/изменение репозитория выполнялись GitHub-коннектором; реальные source/native/browser проверки — в GitHub Actions. Не выдаём эти проверки за локальное исполнение UE. Снимок фактической локальной проверки: [environment.json](Evidence/scaffold/environment.json).

## Репозиторий

Целевая ветка `agent/ue5-core-vertical-slice`; начало `312f21068c6459c878651e0b1d48538c325628b9`. Проверенная реализация `fb2ad5f2d3577050c8518d875ae9cca4c0ff7718`. Перед итоговым отчётом HEAD `a34dab4b91c5981e216e1e4bd9cc84f8d6973441` отличается от неё только workflow экспорта исходников. Финальный отчётный коммит сохраняет этот workflow.

main повторно прочитан: `cda0b460257e656c57b31c8ccebfb79097023d4a`; merge не выполнялся. EngineAssociation пуст, UE5Engine.lock.json не выдуман. Существующие binaries находятся в обычном Git, .gitattributes не менялся. Generated Binaries/Intermediate/Saved/DerivedDataCache/.vs не коммитились.

## Среда фактического CI

Run `34145341913`, job `101816056166`, source `fb2ad5f2d3577050c8518d875ae9cca4c0ff7718`: Ubuntu 24.04.4, GCC 13.3.0, Node 22.23.2, npm 10.9.8. GCC собирал только переносимый C++-домен. UHT/UBT/Editor отсутствуют в цепочке команд этого job.

Подробные команды, результаты и ограничения: [UE5_PROGRESS.md](UE5_PROGRESS.md).

## Доступные инструменты для UE-машины

`ue5.py audit / pin-engine / generate / build`; `scaffold_runner.py build-scene / inspect-scene / automation`; read-only по исходникам Blender-скрипты `extract_city_markers.py` и `prepare_skeletal_lods.py`. Они не означают, что UE-операции уже были исполнены. Сначала требуется реальная установка движка и её pin, затем свежая сборка текущего коммита.

Блокеры: нет доступного установленного UE5/UBT; нет подтверждённого GPU/Editor; нельзя измерить импорт, UHT-совместимость, реальную компиляцию UMG, collision traces, рендер и игровые клики. Эти ограничения не остановили реализацию C++-исходников и проверку переносимого ядра.
