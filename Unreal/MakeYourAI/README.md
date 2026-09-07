# Make Your AI — запуск SOURCE_SCAFFOLD

UE-компиляция, Editor, графика и прохождение NOT VERIFIED. Проверенные native/браузерные результаты — в [UE5_PROGRESS.md](../UE5_PROGRESS.md). Ниже инструкции для машины с настоящим UE5, а не журнал успешно выполненных здесь UE-команд.

## Движок и первая сборка

Используйте Windows или Linux с установленным UE5 и совместимым C++ toolchain. Задайте `UE_ROOT` каталогом, содержащим Engine. Версия не выбрана наугад: EngineAssociation пуст до фиксации реальной установки. Build-wrapper пока не поддерживает macOS.

Из корня репозитория в `agent/ue5-core-vertical-slice`:

```sh
python Unreal/Tools/ue5.py audit
python Unreal/Tools/ue5.py pin-engine
python Unreal/Tools/ue5.py generate
python Unreal/Tools/ue5.py build
```

Pin создаётся по настоящему Engine/Build/Build.version без абсолютного пути установки. Проверьте и закоммитьте этот pin. Для каждой команды проверяйте exit code и свежий отчёт в Saved/Verification. Генерация project files не равна компиляции.

После успешной сборки откройте MakeYourAI.uproject тем же Editor и запустите Play. По умолчанию используется существующая `/Engine/Maps/Entry`; native GameMode создаёт graybox. Он не является CityV4. Временный UMG рассчитан на desktop; используйте окно от 1280 px, затем отдельно проверьте масштабирование интерфейса.

## Garage

Выберите garage, купите локацию, выберите клетку, откройте Procurement & installation. Укажите шасси, чип, канал, количество. Order complete kit списывает цену целиком. Warehouse & orders показывает ETA. Pause останавливает компанию. После доставки выполните Install chassis, затем Install / upgrade chip. Save / load сохраняет текущий слот; после закрытия приложения загрузите тот же слот.

Управление камерой: WASD, колесо мыши, выбор маркера/клетки левой кнопкой. Количество клеток не отменяет питание: Garage имеет девять клеток, но 3 кВт хватает не на девять Terra T1. До доставки монтаж невозможен; отказ по мощности не расходует склад или RNG.

## Реальная сцена CityV4

Сначала извлеките границы исходных объектов через Blender, не изменяя исходник:

```sh
blender --background --python Unreal/Tools/extract_city_markers.py -- --repo-root .
```

Производный JSON находится в MakeYourAI/Saved/ScaffoldSource/city-markers.json. Используется реальная мировая геометрия, а не baked object origins.

После свежей Development Editor сборки:

```sh
python Unreal/Tools/scaffold_runner.py build-scene
python Unreal/Tools/scaffold_runner.py inspect-scene
```

Editor создаст `/Game/Scaffold/Imported`, `/Game/Scaffold/Data/DA_ScaffoldCatalog`, материал и `/Game/Scaffold/Maps/L_Scaffold_City`. Существующая карта не перезаписывается. Повторный импорт разрешён только как использование уже записанного ассета с совпадающим исходным SHA256; чужие/изменённые ассеты не заменяются молча. Исходные FBX/Blend/GLB не изменяются.

Откройте реально созданную карту и запустите Play. DefaultMap не направлен на отсутствующий файл заранее. После проверки выберите карту стартовой вручную. Один combined CityV4 сохраняет композицию, но не обеспечивает пространственную разбивку для streaming; требуется отдельная подготовка чанков/World Partition.

Editor Python API пока не выполнялся в этой среде. Совместимость свойств с вашей UE-версией, единицы, ориентация, UCX, освещение и расстановка требуют проверки. Отсутствие свежего успешного Editor-отчёта считается ошибкой wrapper даже при exit code 0 самого процесса.

## Производные skeletal LOD

```sh
blender --background --python Unreal/Tools/prepare_skeletal_lods.py -- --repo-root .
```

Скрипт читает исходный person-1.fbx и пишет производные FBX только в Saved/ScaffoldSource/person-1-lods. Цели 7500/2000/500, реальные числа появляются в lod-report.json после исполнения. Существующий каталог результата защищён от перезаписи. Проверяйте веса/силуэт и подключайте уровни через Skeletal LOD import в UE. Это не готовая импортированная цепочка и не изменение статичного города.

## Настройка расширений

Auction, АЭС и Greenhaven имеют работающую доменную реализацию, но новые цены/тарифы не утверждены. В реальном DA_ScaffoldCatalog настройте BALANCE_TUNABLE поля и явно включите configured-флаги. Тестовые значения не являются балансом игры. Изменение каталога меняет его отпечаток: старый save потребует миграции или новой компании. Сохранения IndexedDB не импортируются.

## Unreal Automation

Сначала пересоберите текущий commit: старые DLL могут исполнять старый код.

```sh
python Unreal/Tools/ue5.py build
python Unreal/Tools/scaffold_runner.py automation --null-rhi
```

Проверка принимает только свежий index.json, содержащий все восемь MakeYourAI suites с состоянием Success. Нулевое число тестов не считается успехом. NullRHI не подтверждает графику; для визуального и игрового гейта нужен отдельный запуск настоящего RHI и ручной сценарий.

Native, TypeScript и Python команды находятся в `.github/workflows/ue5-scaffold.yml`. Они уже выполнялись, но не заменяют UHT/UBT/UMG-проверку. Cook/package, обнаружение ассетов в packaged game, collision traces, анимация, streaming и FPS остаются открытыми.
