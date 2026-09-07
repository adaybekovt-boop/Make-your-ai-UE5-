# Make your AI — начало игры, Garage, датасеты, обучение и концовки

**Статус: SOURCE_SCAFFOLD. UE5 reference gate: NOT VERIFIED.**

Результат — связанный исходный код с реально исполненными нативными тестами, а не только план или README. Это ещё не собранная Unreal-игра. `COMPILE_VERIFIED` для UE5, `EDITOR_VERIFIED`, `VISUAL_VERIFIED`, `GAMEPLAY_VERIFIED` не присвоены.

## Коммиты и параллельные изменения

Репозиторий `adaybekovt-boop/Make-your-ai-UE5-`, ветка `agent/ue5-core-vertical-slice`. Начальный HEAD задания уже был `fb2ad5f2d3577050c8518d875ae9cca4c0ff7718`, не прежний `312f210`. Базовые игровые подсистемы к этому моменту существовали; дополнение не заменяло их второй экономикой.

| Коммит этого дополнения | Содержание |
| --- | --- |
| `35d5a0bd5256bbd788026d711ee779c9902aad16` | Campaign domain: flow, сложность, три способа review, обучение, пять концовок, codec |
| `f8d48a5d7192fba08cd66f44135afd26e0aa43be` | 43 сценария кампании, отдельный процесс сохранения/продолжения, CI |
| `ca785c27bb888c7a8ca6b4fe01f8dccc5eea3b2f` | GameInstance loading, реальные UMG-классы, SaveGame и Garage walking source |
| `c9c3e0e472a07c3adb2ec41416455747cc102eb9` | Editor bootstrap, Unreal Automation source, Python/source-integrity проверки |
| `23c44e3fdb888dff5f6301a0d1afdaa9a3b790ed` | Исправлено сохранение CI-логов из hidden .out, разрешён только список *.log/fixtures.json |

Параллельно Cursor Agent обновил ту же ветку до `b1fe36bb67ffc1fb08e0ab8cf6d66586c559b19e`: дополнил campaign/core tests, переназвал сложности, расширил payload и UI, добавил исходный портрет. Эти изменения **сохранены**, не приписаны этому прогону и не затёрты устаревшим деревом. Финальная правка поверх них удерживает `InteriorProfiles` живым во время обхода точек Garage и обновляет настоящие хеши/отчёты. SHA этого отчётного коммита можно получить командой `git log -1 --format=%H -- Unreal/UE5_PROGRESS.md`.

`main` не мержился; исходные browser src/package/lock, CityV4 FBX/Blend и исходные модели этим дополнением не заменены. LFS и история не переписывались. Существующий PR не равен merge.

## Что реализовано

Один владелец состояния `UMaiCompanySubsystem` → `mai::Campaign` → прежний `mai::Simulation`: общие деньги, часы, закупки, склад и оборудование. Вся новая логика находится в production C++, который используется UE-адаптерами и напрямую нативными тестами.

Загрузка имеет persistent состояние, обязательные зависимости, реальные async requests, защиту от устаревших callback, ошибку/повтор и неизвестный процент без фальшивой анимации. Новая игра проходит создание компании, выбор профиля, интерактивный пролог и вход на карту. Сложность не меняется случайной кнопкой посреди партии и сохраняется.

Garage имеет C++-сцену небольшого помещения, ACharacter/CharacterMovement, камеру, физическую капсулу и стены, подход к интерактивным объектам, склад, NPC, рабочий стол и выход. Это **runtime graybox**, не проверенная интеграция исходного Garage FBX. Близость и прямая видимость проверяются отдельным кодом взаимодействия. Остальные четыре интерьера описаны данными как следующие кандидаты, но не объявлены ходибельными.

Датасеты приобретаются за деньги и попадают в Unreviewed. Проверка обязательна: ручные 3–5 стадий, человек с наймом/оплатой партии/очередью/усталостью или AI с затратами на создание, резервом реальных вычислений и систематическим bias. Качество, шум, принятая доля, время и решения сохраняются. Юридическое происхождение не очищается проверкой. Обучение использует реальный доступный compute и качество партии, реагирует на поломки и паузу. Нет повторного расходования compute при завершении AI-review внутри временного кванта.

Пять детерминированных концовок с приоритетами: регулятор → банкротство → приобретение Elon Max → независимый глобальный успех → открытая модель. Продажа требует согласия; конечные метрики и журнал решений фиксируются. Новая компания сохраняет старые слоты. Возврат к save допускается только профилем и при наличии играбельного снимка.

Elon Max в этом дополнении имеет собственную fictional-биографию, слот и явный placeholder. Позднее параллельное изменение добавило JPEG; это не результат генерации/импорта данным прогоном и не доказательство готового portrait widget в UE.

## Классы, модели и экраны

Новые нативные классы: `mai::Campaign`, `mai::EndingEvaluator`.

Новые UE-классы: `UMaiCampaignAsset`, `UMaiLoadingSubsystem`, `UMaiFlowWidget`, `UMaiCampaignButton`, `AMaiGarageInterior`, `AMaiWalkCharacter`, `AMaiInteriorPoint`. Расширены `UMaiGameInstance`, `UMaiCompanySubsystem`, `AMaiPlayerController`, `UMaiSaveGame`, `UMaiSaveSubsystem`, старый `UMaiHUDWidget`, `UMaiProximityComponent`.

Модели: `DifficultyProfile`, `DatasetOffer`, `DatasetBatch`, `DatasetItem`, `DatasetInventory`, `DatasetQuality`, `ReviewSession`, `ReviewDecision`, `ReviewMethod`, `Specialist`, `AIReviewer`, `TrainingJob`, `DecisionRecord`, `LoadingState`, `CampaignState`, `EndingProfile`, `EndingMetrics`, `EndingResult`, `FictionalCharacter`, `InteriorPoint`, `InteriorProfile`. UE DataAsset содержит редактируемые структуры Difficulty/DatasetOffer/ReviewCard/Ending.

В исходниках созданы UMG-представления Loading, Main Menu, New Game, Difficulty, Prologue, City Map/Gameplay toolbar, Training и Ending; прежние панели закупки/склада/заказов/установки/сохранений связаны с тем же состоянием. Созданы `.h/.cpp`, которые строят WidgetTree при запуске, а не проверенные designer `.uasset`. В Editor эти экраны не открывались, их качество/адаптивность не подтверждены.

## Сценарий, действительно прошедший от начала до конца

Нативный тест проходит Loading → Main Menu → New Game → Difficulty → Prologue → City Map → покупка Garage → заказ официального rack+chip → доставка → монтаж → Garage state → покупка партии → Unreviewed → четыре ручных решения → Verified → Training → Save → новое создание Campaign/Load → завершение обучения → Ending Evaluation.

Итог стартового сценария при базовых коэффициентах: `cash=2776266667` в единицах 1e-6 доллара, `IQmicro=582179`, четыре сохранённых решения. Оценщик возвращает **нет концовки**, а не искусственный международный успех после одного маленького датасета. Все пять концовок проверены отдельными детерминированными состояниями. Полный переход в Ending и обратно к новой компании проверен отдельным тестом с явно тестовыми порогами; production-пороги ради демонстрации не снижались.

Вторая проверка действительно запускает новый нативный процесс, читает сохранение, продвигает игру и сравнивает выход побайтно с непрерывным прогоном. Это доказательство нативного persistence, не имитация открытия UE Editor и не ручная живая партия.

## Выполненные проверки и доказательства

Первый полный CI пакета: `c9c3e0e472a07c3adb2ec41416455747cc102eb9`, [run 34150885534 / job 101832744620](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34150885534/job/101832744620): 28 базовых сценариев / 848 assertions и 43 campaign / 1549 assertions, browser 388, parity 347, Python 16+23 — прошли. У этого прогона выгрузка hidden .out сначала пропустила артефакт, хотя лог существовал; ошибка CI-конфигурации исправлена отдельным коммитом.

Объединённый код проверен на `b1fe36bb67ffc1fb08e0ab8cf6d66586c559b19e`, [run 34151292202 / job 101833955477](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34151292202/job/101833955477). Прочитан полный job log, скачан и распакован [artifact 10029461032](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34151292202/artifacts/10029461032).

| Команда/проверка | Результат | Exit |
| --- | --- | ---: |
| g++ C++17, -Wall/-Wextra/-Werror, ASan/UBSan, native main | 46 сценариев, 1 355 assertions, 0 ошибок | 0 |
| bash Unreal/Tests/run_campaign.sh | 43 сценария, 1 549 assertions, 0 ошибок | 0 |
| Два процесса + cmp сохранённого продолжения | CROSS_PROCESS_RESTART_PASS | 0 |
| npm ci | Установка из неизменённого lock | 0 |
| npm test | 388/388, 29 файлов | 0 |
| npm run typecheck | Успешно | 0 |
| npm run build | Успешно; прежнее предупреждение Draco chunk 615.55 kB | 0 |
| npx vitest run --config Unreal/Tests/vitest.config.ts | 347 C++/TypeScript parity tests | 0 |
| python3 -m unittest discover -s Unreal/Tools/tests -v | 16 preparation tests | 0 |
| python3 -m unittest discover -s Unreal/Tests -p 'test_*.py' -v | 23 source/runner tests | 0 |
| Локальный scaffold_runner.py build-campaign без UE_ROOT | Отказ до запуска Editor; process_exit_code=null | 2 |

Последняя строка — ожидаемая блокировка, не успешный UE build. 89 нативных сценариев и 2 904 assertions после объединения не равны 89 уникальным игровым механикам: наборы частично проверяют одинаковые границы.

Сводка с привязкой к SHA: [Evidence/campaign/ci-summary.json](Evidence/campaign/ci-summary.json). Манифест [source-hashes.json](Evidence/campaign/source-hashes.json) проверяет реальные байты опубликованных исходников. Локальные GCC-проверки выполнялись отдельно в распакованном снимке; браузерные проверки — на настоящем CI checkout.

## Что осталось непроверенным и известные ограничения

UE5/UHT/UBT отсутствуют: возможные ошибки Unreal API, reflection, linking и WidgetTree выявляются только настоящей сборкой. Тесты `EditableProfilesMatchDomain`, `FullFlowRealSaveGame`, `GarageSweptMovementAndInteraction` написаны для Unreal Automation и НЕ запускались. Source-тест, который нашёл вызов LineTrace, не доказывает корректную физику.

Ни новых `.umap/.uasset`, ни UE-скриншотов в этом прогоне нет. Editor-скрипты только подготовлены. Непрерывное отображение загрузки во время hard travel, реальная видимость streamed Garage, корректность освещения, UI на разных разрешениях, анимации, Nanite/Lumen/WP/LOD и FPS требуют UE-машины.

Полная браузерная бизнес-симуляция не перенесена: коммерческая связь нового model-quality с портфелем, аудиторией, контрактами и всеми конкурентными событиями остаётся работой по интеграции. Новые профили/услуги review/пороги концовок не прошли живую балансную партию 1×. Аукцион/АЭС/Greenhaven требуют утверждённых tunables; disabled-конфигурация остаётся намеренной, а не скрытой бесплатной экономикой.

После параллельного обновления названия сложностей в текущем HEAD — **Easy / Normal / Hard**, тогда как это задание требует **Startup / Standard / Hardcore**. До `23c44e3` использовались имена задания. Расхождение не исправлялось слепой заменой ID, чтобы не разрушить сохранения и новый набор тестов. Требуются согласованные display labels/aliases и миграция. Параллельный UI/source пакет сохранён, но не выдаётся за полностью проверенное сочетание экранов в UE.

Image-review — подписанные карточки, а не набор реальных фотографий. Исходный fictional portrait, добавленный другим прогоном, не прошёл здесь импорт/визуальный осмотр в UE. Схемы нативных сохранений имеют миграции; браузерный IndexedDB-формат не объявлен поддержанным.

## Следующий конкретный шаг

На доступной UE5-машине зафиксировать установленную версию, выполнить UHT/Development Editor, затем `scaffold_runner.py build-campaign` и `automation`. После успешной сборки пройти настоящий Garage → dataset review → training → SaveGame/reload → Ending Evaluation, проверить коллизии и интерфейс. До этого сохранять статусы SOURCE_SCAFFOLD / UE5 NOT VERIFIED, независимо от зелёных нативных тестов.
