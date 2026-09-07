# UE5 reference gate — после связанного каркаса кампании

**UE5 reference gate: NOT VERIFIED**

Источник проверок кампании: `c9c3e0e472a07c3adb2ec41416455747cc102eb9`; объединённый source CI: `b1fe36bb67ffc1fb08e0ab8cf6d66586c559b19e`. Reference gate по обновлённому заданию не мешает писать исходники, но не считается пройденным.

| Проверка | Фактическое состояние |
| --- | --- |
| UE5 version / UHT / UBT | Установка отсутствует; версия не зафиксирована, UE compile не запускался |
| L_Reference_GarageCity / L_Scaffold_City | Существующий Editor builder сохранён; исполнение в UE не подтверждено |
| L_Campaign_Boot / L_Campaign_Garage | Добавлен build_campaign.py; реальные .umap через Editor API этим прогоном не созданы |
| Garage FBX / enclosure collision | Исходники сохранены; runtime walking использует явный graybox, не выдаётся за проверенный FBX |
| Nanite architecture | Подготовка импортного pipeline, не проверка Nanite в Editor |
| Lumen / dynamic shadows | Нет реального UE-рендера и настройки качества на целевой GPU |
| World Partition | Нет измеренных параметров готового мира; preload/travel код не доказывает streaming cells |
| ISM/HISM/Foliage | Исходный runtime/import scaffold сохранён; GPU/draw-call измерений нет |
| Skeletal LOD0/LOD1/LOD2 | Импортированные количества треугольников неизвестны; целевые 5–8k / ~2k / ~500 не считаются достигнутыми |
| Walking / floor / wall sweep | ACharacter, CharacterMovement и отдельный Automation-тест написаны; реальная UE-физика не запускалась |
| Interaction line traces | Реальные команды и тестовый код есть, Editor execution отсутствует |
| Loading / UMG transitions | C++ состояния, асинхронные UE API и WidgetTree-классы есть; визуальная непрерывность переходов не проверена |
| SaveGame disk | Есть USaveGame write/read Automation test; выполнен только нативный процессный roundtrip, не UE-disk test |
| Full clicked Garage/data/training/ending flow | Native state scenario проходит; GAMEPLAY_VERIFIED в UE не присвоен |

В исходных моделях 1 Blender unit = 1 м, в UE используется сантиметровая сцена. Не применять дополнительный ×100 без измерения реального импорта. Исходные персонажи и автомобили смотрят -Y при Z-up; требуются отдельные проверки направления, масштаба и физики. Статичный CityV4 не подвергался новой decimation ради браузерных ограничений.

Нельзя использовать Blender-картинки или новый portrait-source как доказательство Unreal сцены. Скриншоты UE5 этим прогоном не создавались. EngineAssociation/engine pin должен определяться установленной машиной, не названием проекта.

После UHT/Development Editor: выполнить build-campaign и исходный CityV4 reference import, запустить весь актуальный список MakeYourAI Automation tests, затем проверить полноценное прохождение мышью/клавиатурой. Для closed gate нужны реальные map files, логи, trace/sweep результаты, LOD counts, параметры WP и рендеры с target GPU. Наличие .cpp, .py и зелёных браузерных тестов недостаточно.
