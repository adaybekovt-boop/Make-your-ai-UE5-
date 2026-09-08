# Browser → UE correspondence

Browser authority: `fd270196525c4c6fe61217327c81274e67a58d4b`; UE base: `cf087025697e9ba3e3ab3ca5380bcdee54edea3d`.

**This is an incomplete migration. Source implementation is not UE runtime acceptance. Every UE/GPU/UI row remains unverified until executed.**

The inventory below is backed by the checked-out source; `browser-inventory.json` lists every store action and SHA-256 of every source file. No original browser prices, catalog entries, or source functions were edited. Native-only features are identified rather than attributed to the browser.

| Browser function/screen | Source | UE implementation | Open gap | Verification |
|---|---|---|---|---|
| Loading / ready / storage errors | src/ui/App.tsx; store.initialize | MaiLoadingSubsystem + native rule startup; async assets/streamed city; failed/cancel/retry | UE execution, shader compilation progress and cancellation UI not run | UE loader failure/cancel tests; real screenshots |
| Main menu / Continue / new setup | MainMenu.tsx; NewGameScreen.tsx | Native menu + setup, persisted hasSave | Visual fidelity not accepted | 3 viewport captures, continue from fresh process |
| Flagship / portfolio strategy, model name | NewGameScreen.tsx; startNewGame | Original source command, pure UMG form | UE text focus and validation not run | Differential two-strategy trace + setup capture |
| Difficulty / Prologue | No matching browser screens; existing UE extension | Existing immutable difficulty/prologue route attached to source game start | Only capital/hiring/legal grace reliably bridged; difficulty training/failure/competitor modifiers NOT full parity | Native 46-case suite; UE flow + balance review |
| Settings / volume / fullscreen / UI scale | App.tsx settings; native-only volume/fullscreen additions | UMG fields, engine settings, safe per-user config | Reload settings and original map-control options require acceptance | Fresh process settings test; 720/900/1080 |
| Quit | Browser session close; no native Quit action | Native QuitGame with durable save failure guard | UE process-close path not executed | Real capture/resume runner + manual close |
| Map selection / camera / reset view | MapView.tsx; RegionMap.ts; mapNavigation.ts | Authored CityV4 camera; UE pan/zoom + location cards/click targets | Exact browser drag/home behavior and camera hit tests incomplete | UE daytime full-city and point captures |
| Nine mainland sites / location requirements | city.ts; config.ts; LocationCard.tsx | Original purchaseLocation, location detail/access gates | Physical interiors beyond Garage not connected | All original city tests + UE purchase denial cases |
| Regions / remote sites / global network | city.ts; propertyExpansion.test.ts; App.tsx | Original unlockRegion and region purchases, native region cards | Regional geography not recreated as UE worlds | Source regression tests; region action UI |
| Head office / city towers / property rent | city.ts; OfficeScreen.tsx | Original purchaseOffice / purchaseCityTower and rent math; management UI | Authored office walkable scene unavailable | Source property tests + office/purchase UI |
| Procurement chassis / chips / kits / channels | ProcurementModal.tsx; procurement.ts | Original catalogs/orderEquipment/orderKit; no duplicated prices | UE controls/cell targeting unverified | Differential invalid compatibility and valid kit |
| Delivery / fraud / expediting / warehouse | procurement.ts; economy.ts | Original stochastic and time rules, stock/queue readouts | 3D delivery animation not copied | 1x differential delivery; fraud stochastic test |
| Install / swap / upgrade / placement / capacity | InteriorScreen.tsx; serverGrid.ts; equipment.ts | Original mountChassis/mountChip; native grid/cell/stock selection | Authored equipment visuals not imported by new city builder | Original grid/procurement tests; UE real-click grid |
| Sell / reserve / redeploy / overclock | InteriorScreen.tsx; models/commands.ts | Original sellAt/deployReserve/overclockAt, native controls | UE point selection not executed | Server grid regression tests |
| Garage movement / E / desks / lockers | No browser third-person walking; existing UE extension | Existing swept Character movement and proximity interaction retained | Avatar and room remain explicit graybox; no mouse-look yet | Original movement Automation + human walk test |
| Other interiors / office / staff / NPC | InteriorScene.ts; StreetLife.ts; existing UE NPC extension | Procedural garage NPC interactions preserved | Other 3D interiors, animated pedestrians/traffic and portrait assets are not ported | Manual world review; source StreetLife tests do not prove UE |
| Dataset purchase / origin / domain / inventory | TrainingScreen.tsx; models/commands.ts | Original paid lots; canonical lot -> native review batch without second payment | Native extension 10-unit/image/mixed offers not exposed by new default UI | Differential lot purchase + native paid-batch regression |
| Manual / Human / AI review | No browser equivalent; existing UE extension | Review controls, specialist/AI costs; manual requires desk; review compute reserved once | Quality metrics do not become new browser price/training formulas; image/mixed UI gap remains | Native review tests + UE desk/review sequence |
| Training / compute / source quality / queue | TrainingScreen.tsx; training.ts; models/simulation.ts | Original training; unreviewed-lot guard; source compute budget less actual review reservation | UE training completion/UI not run; native extension training formulas not substituted | Differential start/ticks; original training tests; UE wait at 1x |
| Model catalog / Aurora S3 / Vertex C7 / Meridian M13 | CatalogScreen.tsx; models/config.ts | Original bases/costs/debt/requirements; native cards | Art/typography differ from browser | Original model tests; UE catalog buttons |
| Flagship irreversible replacement / rename | CatalogScreen.tsx; ModelTabs.tsx | Original purchaseBase replace flag + confirmation form; renameSelected | Exact confirmation/close behavior pending UI review | Original tests + destructive-action UI check |
| Portfolio / model tabs / allocations / normalization | PortfolioScreen.tsx; ModelTabs.tsx | Original setAllocations + shared compute/economy; all models select | UE layout under many models not run | Differential portfolio trace; original allocation tests |
| Quantization / memory / quality / training efficiency | QuantizePanel.tsx; models/commands.ts | Original steps and constraints; native selected-model controls | Modal close/focus behavior unverified | Original model tests; quant UI negatives |
| Personality / friendly or raw / immutable decision | App.tsx personality dialog | Original setPersonality and source descriptions | Native dialog visual match pending | Differential friendly; source tests; UE screenshot |
| Benchmark / preparing / cheating / rank | TestingScreen.tsx; benchmark.ts | Original runBenchmark/togglePreparing and displayed metrics | Presentation not pixel-equivalent | Original benchmark tests + UI |
| Market / users / demand / token pricing / ads | TestingScreen.tsx; market.ts; economy.ts | Original source functions; native sliders/charts | Graph and tooltip visuals differ | Original market/economy tests; differential frames |
| Contracts / official / grey / enterprise / lifecycle #3 | contracts.ts; App.tsx; contract-lifecycle.test.ts | Original accept/decline, selected model, all source obligations/time boundaries | Actual UE offer dialog and failure event not exercised | Existing lifecycle assertions unchanged + human 1x run |
| Licensing / open-source irreversible choice | TestingScreen.tsx; App.tsx; models/commands.ts | Original toggleLicense/chooseOpenSource; native controls/confirmation | UE destructive-choice UX unverified | Source model/market tests + UI |
| Team / safety / engineer / hiring / firing / overwork | team.ts; OfficeScreen.tsx | Original staff commands; review specialists separate explicit extension | Native staffing visual cards differ | Original team tests + review hire tests |
| Tech tree / unlocks / prerequisites | App.tsx; config.ts; models/commands.ts | Original unlockTech and tree entries | Native tree currently cards, not original layout | Source tests + UI locked-node checks |
| Competitor / espionage / reputation | competitor.ts; reputation.ts; App.tsx | Original attemptEspionage/reputation events | Character artwork/NPC ties incomplete | Original competitor/reputation tests |
| Insurance / risks / fire / court / failure / downtime | events.ts; daily.ts; economy.ts | Original risk/stochastic/day-boundary rules; native warnings extension | No humanPlaytest; shared extension legal risk needs UE balance acceptance | Original event/economy regressions; human 1x |
| Finance / cash / capex / debt / expenses / property income | models/economy.ts; App.tsx | Single browser ledger; native projection, no second tick/revenue stream | Compatibility limits retained from old native domain (cash etc.) narrower than browser | 568852 differential assertions; bounds rejection tests |
| Pause / 1x / 3x / clock / day transitions | useGameRuntime.ts; daily.ts | Native frame driver calls source tick; suspends during load/background | Background behavior differs from browser offline wall-clock handling | 1x primary differential trace plus 3x path |
| Event log / notifications / history charts | NoticeToast.tsx; App.tsx; gameStore.ts | Source event messages retained; UMG notices + Slate series | UE toast timing and all chart labels not visually validated | Pure view/RNG tests; visual review |
| Bankruptcy / source acquisition ending | models/simulation.ts; App.tsx | Original source ending plus native terminal screen and clean setup | Five legacy native endings use legacy threshold projections; full V3 acceptance not done | Source simulation/native ending tests; UE terminal -> New Game |
| Additional native endings / decisions / allowed return | Existing UE Campaign definitions, not browser | Native title/narrative/decision history retained | Technical decision identifiers need user-facing localization; some old offers unavailable | Native 46-case suite; manual terminal UX |
| Auction / Nuclear Power Station / Greenhaven | CityV4 visual labels, no browser transaction rules | Geography retained; UI denies nonexistent browser transactions explicitly | No authoritative gameplay rule to port; old extension functionality not fully connected | Compare source inventory; no invented prices/rules |
| Save / autosave / close / restart / Load | companySaves.ts; saves.ts; gridSave.ts | Original V0–V3 codec; native envelope/checksum/bounded atomic files/backups; transactional world load | Old UE SaveGame slots not migrated; UI selection/camera restore incomplete | Actual native two-process tests + runner fresh UE process |
| Corrupt / invalid / cancelled / failed load | companySaves.test.ts; native loader | Validate before commit; rollback prior state; no save overwrite on rejection | UE travel-failure/cancel integration not executed | Malformed VM save tests; real UE cancel/failure test |
| CityV4 unique geometry / instances / source assets | Unreal/CityV4; Unreal/Assets | Read-only Blender exporter; full evaluated geometry, UV/normals/colors; HISM groups | New v2 vertex-color exporter awaits its own measured run; UE LOD0 unknown | Source export twice; manifest checks; UE audit per mesh |
| Water / glass / PBR / source materials | CityV4 source material graphs | Actual UE graph/MI builder: source base/vertex color, roughness, metal, emission, alpha; usage flags | Procedural Noise->Bump normals not copied; GPU/shader graphs not executed | Source graph manifest + UE shader compile + material closeups |
| Day/night / exposure / lights / shadows | CityV4 DAY/NIGHT source and render refs | One Sun/Sky rig; actual authored camera; requested DX12 SM6 Lumen VSM | Light intensity/exposure are unverified baseline presets, not source-exact output | Real UE day/night screenshots, no Blender substitution |
| Nanite / fallback / LOD0 / streaming / HLOD | Source topology + unknown user local import | Default full-detail non-Nanite; opt-in full fallback; explicit reduction/audit; spatial HISM | Cause of old triangle damage UNKNOWN; no GPU perf/WP/HLOD acceptance | UE source-vs-LOD0 audit first, GPU baseline then optimize |
| UI overlap / DPI / anchors / clipped toolbar | styles.css + former MaiHUD/MaiFlowWidget | One active native widget, wrapping top bar, bounded panels, stable widgets/scroll/input | 1280x720/1600x900/1920x1080 not executed; not declared matching | Runner captures all sizes and checks actual PNG/control bounds |

## Store action coverage

50 callable store members inventoried; 44 directly allowlisted, lifecycle/persistence methods mapped to the native host.

| Browser action | Native route |
|---|---|
| `initialize` | Native lifecycle / tick / durable persistence adapter |
| `continueGame` | Native lifecycle / tick / durable persistence adapter |
| `beginSetup` | Original `gameStore.ts` command through `MaiRulesVM` |
| `cancelSetup` | Original `gameStore.ts` command through `MaiRulesVM` |
| `startNewGame` | Original `gameStore.ts` command through `MaiRulesVM` |
| `selectLocation` | Original `gameStore.ts` command through `MaiRulesVM` |
| `selectModel` | Original `gameStore.ts` command through `MaiRulesVM` |
| `sellAt` | Original `gameStore.ts` command through `MaiRulesVM` |
| `overclockAt` | Original `gameStore.ts` command through `MaiRulesVM` |
| `deployReserve` | Original `gameStore.ts` command through `MaiRulesVM` |
| `purchaseOffice` | Original `gameStore.ts` command through `MaiRulesVM` |
| `purchaseCityTower` | Original `gameStore.ts` command through `MaiRulesVM` |
| `purchaseLocation` | Original `gameStore.ts` command through `MaiRulesVM` |
| `openProcurement` | Original `gameStore.ts` command through `MaiRulesVM` |
| `closeProcurement` | Original `gameStore.ts` command through `MaiRulesVM` |
| `orderEquipment` | Original `gameStore.ts` command through `MaiRulesVM` |
| `orderKit` | Original `gameStore.ts` command through `MaiRulesVM` |
| `mountChip` | Original `gameStore.ts` command through `MaiRulesVM` |
| `mountChassis` | Original `gameStore.ts` command through `MaiRulesVM` |
| `tick` | Native lifecycle / tick / durable persistence adapter |
| `togglePause` | Original `gameStore.ts` command through `MaiRulesVM` |
| `setSpeed` | Original `gameStore.ts` command through `MaiRulesVM` |
| `persist` | Native lifecycle / tick / durable persistence adapter |
| `restore` | Native lifecycle / tick / durable persistence adapter |
| `reset` | Native lifecycle / tick / durable persistence adapter |
| `dismissNotice` | Original `gameStore.ts` command through `MaiRulesVM` |
| `buyDataLot` | Original `gameStore.ts` command through `MaiRulesVM` |
| `startTraining` | Original `gameStore.ts` command through `MaiRulesVM` |
| `setPersonality` | Original `gameStore.ts` command through `MaiRulesVM` |
| `runBenchmark` | Original `gameStore.ts` command through `MaiRulesVM` |
| `togglePreparing` | Original `gameStore.ts` command through `MaiRulesVM` |
| `acceptContract` | Original `gameStore.ts` command through `MaiRulesVM` |
| `declineContract` | Original `gameStore.ts` command through `MaiRulesVM` |
| `hireEmployee` | Original `gameStore.ts` command through `MaiRulesVM` |
| `fireEmployee` | Original `gameStore.ts` command through `MaiRulesVM` |
| `toggleOverwork` | Original `gameStore.ts` command through `MaiRulesVM` |
| `toggleAdvertising` | Original `gameStore.ts` command through `MaiRulesVM` |
| `setTokenPrice` | Original `gameStore.ts` command through `MaiRulesVM` |
| `toggleInsurance` | Original `gameStore.ts` command through `MaiRulesVM` |
| `toggleLicense` | Original `gameStore.ts` command through `MaiRulesVM` |
| `chooseOpenSource` | Original `gameStore.ts` command through `MaiRulesVM` |
| `unlockTech` | Original `gameStore.ts` command through `MaiRulesVM` |
| `attemptEspionage` | Original `gameStore.ts` command through `MaiRulesVM` |
| `unlockRegion` | Original `gameStore.ts` command through `MaiRulesVM` |
| `acceptAcquisition` | Original `gameStore.ts` command through `MaiRulesVM` |
| `declineAcquisition` | Original `gameStore.ts` command through `MaiRulesVM` |
| `renameSelected` | Original `gameStore.ts` command through `MaiRulesVM` |
| `purchaseBase` | Original `gameStore.ts` command through `MaiRulesVM` |
| `setModelQuantization` | Original `gameStore.ts` command through `MaiRulesVM` |
| `setAllocations` | Original `gameStore.ts` command through `MaiRulesVM` |
