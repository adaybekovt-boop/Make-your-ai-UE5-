import type { useGameStore } from '../../src/store/gameStore'
import * as cfg from '../../src/systems/config'
import { CITY_TOWERS, CHIP_SHOPS, OFFICE_PRICE } from '../../src/systems/city'
import { orderPrice, deliveryHours, chassisSupports } from '../../src/systems/procurement'
import { normalizeLocation, locationDefinition, firstFreeCell, sameCell, serverOutput } from '../../src/systems/serverGrid'
import { portfolioEconomy, portfolioLocationEconomy, companyUsers, effectiveProfile, modelIsOnline, benchmarkIsCurrent } from '../../src/systems/models'
import { BASE_MODELS, CATEGORIES, CATEGORY_LABELS, DOMAIN_LABELS, QUANTIZATION_LABELS, modelDisplayName, baseCatalogLabel, catalogWeightGb, startingGmi, quantizationPreview, domainGainPreview, flagshipReplaceBlockers, modelIdleStatus, remainingAllocationBps } from '../../src/ui/modelView'
import type { AnyLocationId, ChipId, ChassisId, Channel, GridPosition } from '../../src/systems/types'
import type { DataDomain, QuantizationStep } from '../../src/systems/models'

type Store = ReturnType<typeof useGameStore.getState>
export interface Node {
  kind: 'text'|'heading'|'button'|'input'|'select'|'slider'|'progress'|'ring'|'row'|'column'|'card'|'chart'|'bar'|'spacer'|'grid'|'columns'
  id: string; label?: string; value?: string|number; role?: string; enabled?: boolean
  action?: string; args?: unknown[]; options?: {label: string;value: string}[]; icon?: string; tooltip?: string
  min?: number; max?: number; step?: number; children?: Node[]; samples?: number[]; columns?: number
}
export interface HostView {
  hasSave?: boolean; loading?: {status: string; progress: number|null; cancel: boolean; failed: boolean}
  overlay?: Node; screen?: string; walking?: boolean; review?: Node; companyName?: string
  canReturn?: boolean; ready?: boolean; saveError?: string; inputHint?: string
}
export interface UiState {
  page: string; modal: string; fields: Record<string,string>; location: AnyLocationId
  cell: GridPosition|null; selectedServer: string|null; orderKind: 'kit'|'chip'|'chassis'
  chip: ChipId; chassis: ChassisId; channel: Channel; qty: number; domain: DataDomain
}
export function initialUi(): UiState {
  return {page:'map',modal:'',fields:{strategy:'flagship',name:'',rename:'',baseName:'',base:'terra-s3',quant:'0'},
    location:'garage',cell:null,selectedServer:null,orderKind:'kit',chip:'consumer-gpu',chassis:'rack-basic',channel:'official',qty:1,domain:'general'}
}
const n = (value: number, digits = 0) => Number.isFinite(value) ? value.toFixed(digits).replace(/\B(?=(\d{3})+(?!\d))/g,' ') : '—'
const money = (value: number) => '$'+n(value)
const text = (id: string, label: string, role='body'): Node => ({kind:'text',id,label,role})
const head = (id: string,label: string): Node => ({kind:'heading',id,label})
const button = (id: string,label: string,action: string,args: unknown[]=[],enabled=true,role='secondary'): Node => ({kind:'button',id,label,action,args,enabled,role})
const row = (id: string,...children: Node[]): Node => ({kind:'row',id,children})
const column = (id: string,...children: Node[]): Node => ({kind:'column',id,children})
const card = (id: string,...children: Node[]): Node => ({kind:'card',id,children})
const input = (id: string,label: string,value: string): Node => ({kind:'input',id,label,value,action:'ui:field',args:[id]})
const select = (id: string,label: string,value: string,options: {label:string;value:string}[]): Node => ({kind:'select',id,label,value,options,action:'ui:field',args:[id]})
const slider = (id: string,label: string,value: number,min:number,max:number,step:number,action:string,args:unknown[]=[]): Node => ({kind:'slider',id,label,value,min,max,step,action,args})
const progress = (id:string,label:string,value:number):Node => ({kind:'progress',id,label,value:Math.max(0,Math.min(1,value))})
const page = (id: string,label: string) => button('nav-'+id,label,'ui:page',[id])
const iconButton = (node: Node, icon: string): Node => ({...node, icon, tooltip: node.label})
const profile = (id:string,values:Record<(typeof CATEGORIES)[number],number>) => row(id,...CATEGORIES.map(k=>text(id+'-'+k,CATEGORY_LABELS[k]+' '+n(values[k],1))))

/** Pure presentation. Never advances time, calls RNG, touches a file or changes game state. */
export function makeView(s: Store, ui: UiState, host: HostView = {}) {
  const c=s.company, g=c.company, e=portfolioEconomy(c)
  const m=c.models.find(v=>v.id===s.selectedModelId) ?? c.models[0]
  const rates=e.models.find(v=>v.modelId===m.id)!
  const title=host.companyName||'Make Your AI'
  let content: Node, mode='world', modal: Node|null=null
  const close=button('close-dialog','Закрыть','ui:modal',[''])
  if (host.loading) {
    const l=host.loading
    content=column('loading',head('brand','Neuron'),text('project',title),text('operation',l.status),
      l.progress===null ? text('waiting','Загрузка…') : progress('load-progress',n(l.progress*100)+'%',l.progress),
      row('load-actions',button('cancel-load','Отмена','host:cancel',[],l.cancel),button('retry-load','Повторить','host:retry',[],l.failed)))
    return {schema:1,mode:'full',page:'loading',content,toolbar:null,modal:null,notice:null}
  }
  if (host.overlay) return {schema:1,mode:'full',page:host.screen||'campaign',content:host.overlay,toolbar:null,modal:null,notice:host.saveError||s.notice?.message||null}
  if (s.phase==='menu') {
    content=column('menu',head('brand','Neuron.'),text('tagline','Лаборатория, серверы и модели — в одном городе. Выбор стратегии делается один раз.'),
      button('new-game','Новая игра','beginSetup',[],s.ready,'primary'),
      button('continue','Продолжить','host:load',[],!!host.hasSave),
      button('settings','Настройки','ui:modal',['settings']),button('quit','Выйти из игры','host:quit'))
    mode='full'
  } else if (s.phase==='setup') {
    content=column('new-game',text('setup-eyebrow','Создание компании','muted'),head('new-title','Стратегия партии'),text('strategy-explainer','Этот выбор нельзя изменить посреди партии.'),
      button('strategy-flagship','Один флагман','ui:field',['strategy','flagship'],true,ui.fields.strategy==='flagship'?'selected':'secondary'),
      text('flagship-description','Одна модель владеет всем вычислительным бюджетом. Так устроена текущая игра.'),
      button('strategy-portfolio','Портфель моделей','ui:field',['strategy','portfolio'],true,ui.fields.strategy==='portfolio'?'selected':'secondary'),
      text('portfolio-description','До четырёх моделей с явным разделением мощности. Новые базы покупаются отдельно и стартуют без квоты.'),
      input('name','Название флагмана',ui.fields.name),
      text('name-limit','От 1 до 48 символов. Купленная база и заработанный обучением IQ учитываются отдельно.'),
      row('setup-actions',button('setup-back','Назад','cancelSetup'),button('setup-next','Продолжить','host:setup-next',[],ui.fields.name.trim().length>0&&ui.fields.name.trim().length<=48,'primary')))
    mode='full'
  } else if (g.ending) {
    mode='full';content=column('ending',head('ending-title','Продажа компании'),text('ending-scene','Предложение принято. Компания переходит новому владельцу.'),
      text('ending-stats',`${money(g.cash)} · Выручка ${money(g.totalRevenue)} · Расходы ${money(g.totalExpenses)} · Пользователи ${n(companyUsers(c))}`),
      text('ending-decisions',`Стратегия: ${c.strategy==='portfolio'?'Портфель':'Флагман'} · Моделей: ${c.models.length} · Репутация: ${n(g.reputation)}`),
      ...c.models.map(v=>text('result-'+v.id,`${modelDisplayName(v)}: IQ ${n(v.state.iq,1)} · ${QUANTIZATION_LABELS[v.quantization]} · ${v.state.openSource?'Открытая':'Закрытая'} модель`)),
      row('ending-actions',button('ending-new','Новая игра','beginSetup',[],true,'primary'),button('ending-load','Вернуться к сохранению','host:load',[],!!host.canReturn),button('ending-menu','Главное меню','host:menu')))
  } else {
    const modelTabs=row('model-tabs',...c.models.map(v=>button('model-'+v.id,modelDisplayName(v),'selectModel',[v.id],true,v.id===m.id?'selected':'secondary')))
    const modelTitle=column('model-title',modelTabs,head('model-name',modelDisplayName(m)),
      text('model-base',`${baseCatalogLabel(m)} · ${QUANTIZATION_LABELS[m.quantization]} · ${modelIdleStatus(c,m).label}`),
      {kind:'grid',id:'model-metrics',columns:3,children:[
        card('iq-card',text('iq-label','ЗАРАБОТАННЫЙ IQ','muted'),head('model-iq',n(m.state.iq,1))),
        card('compute-card',text('compute-label','МОЩНОСТЬ МОДЕЛИ','muted'),head('model-compute',n(rates.allocatedCompute,2))),
        card('users-card',text('users-label','ПОЛЬЗОВАТЕЛИ','muted'),head('model-users',n(m.users)))
      ]},profile('effective-gmi',effectiveProfile(m)))
    if (ui.page==='training') {
      mode='panel'
      const run=m.state.run, volume=m.state.queue.reduce((sum,v)=>sum+v.volume,0)
      const live=effectiveProfile(m), purchased=startingGmi(m)
      const competence:Node={kind:'columns',id:'training-competence',children:[column('training-summary',modelTabs,
        text('training-eyebrow','ОБУЧЕНИЕ · '+modelDisplayName(m),'muted'),
        head('training-earned-iq','Заработанный IQ '+n(m.state.iq,1)),
        text('training-model-status',modelIdleStatus(c,m).label)),
        {kind:'grid',id:'training-categories',columns:2,children:CATEGORIES.map(k=>card('competence-'+k,
          text('competence-label-'+k,CATEGORY_LABELS[k],'muted'),head('competence-live-'+k,n(live[k],1)),
          text('competence-base-'+k,'Стартовый GMI '+n(purchased[k],1),'muted')))}]}
      content=column('training',competence,
        card('dataset-store',head('datasets','Партии данных'),text('domain-label','ДОМЕН ПАРТИИ','muted'),
          row('domain',...Object.entries(DOMAIN_LABELS).map(([value,label])=>button('domain-'+value,label,'ui:field',['domain',value],true,ui.domain===value?'selected':'secondary'))),
          column('data-offers',
            iconButton(button('official-data',`Официальные данные · ${cfg.DATA_LOT_OFFICIAL.volume} ед. · ${money(cfg.DATA_LOT_OFFICIAL.price)}`,'buyDataLot',['official',ui.domain],g.cash>=cfg.DATA_LOT_OFFICIAL.price),'flask'),
            text('official-safety','Проверенное происхождение. Без риска заражения.','muted'),
            iconButton(button('unofficial-data',`Данные с рынка · ${cfg.DATA_LOT_UNOFFICIAL.volume} ед. · ${money(cfg.DATA_LOT_UNOFFICIAL.price)}`,'buyDataLot',['unofficial',ui.domain],g.cash>=cfg.DATA_LOT_UNOFFICIAL.price),'flask')),
          profile('data-forecast',domainGainPreview(ui.domain,cfg.DATA_LOT_OFFICIAL.volume)),text('data-risk','Официальные и неофициальные данные имеют разное происхождение и риск заражения.')),
        card('dataset-inventory',head('inventory','Очередь данных'),...m.state.queue.map(l=>text('lot-'+l.id,`#${l.id} · ${l.quality==='official'?'Официальные':'Неофициальные'} · ${DOMAIN_LABELS[l.domain]} · ${l.volume} ед.`)),text('queue-volume',`В очереди: ${volume} ед.`),
          button('open-review','Проверка данных','host:review')),
        host.review??column('review-state'),
        card('training-run',head('training-title','Обучение модели'),
          {kind:'ring',id:'training-progress',value:run&&run.total>0?Math.max(0,Math.min(1,1-run.remaining/run.total)):-1,
            label:run?`Загрузка в ${modelDisplayName(m)}`:volume>0?'Данные в очереди':'Ожидание данных'},
          text('training-idle',run?`${n(run.total-run.remaining,1)} / ${n(run.total)} ед.`:volume>0?'Данные готовы. Проверьте партии перед запуском.':'Приобретите первую партию справа.'),
          text('training-rate',`${n(rates.trainingVolumePerHour,2)} ед./игровой час`),iconButton(button('train','Начать обучение','host:start-training',[],!run&&volume>0&&modelIsOnline(c,m),'primary'),'play')),
        card('team',head('team-title','Команда'),text('morale',`Мораль ${n(g.team.morale)} · Сотрудников ${g.team.employees.length}/${cfg.MAX_EMPLOYEES}`),
          row('hire',button('hire-engineer',`Инженер · ${money(cfg.HIRE_COST.engineer)}`,'hireEmployee',['engineer']),button('hire-safety',`Специалист безопасности · ${money(cfg.HIRE_COST.safety)}`,'hireEmployee',['safety'])),
          ...g.team.employees.map(v=>row('employee-'+v.id,text('employee-label-'+v.id,`${v.role==='engineer'?'Инженер':'Безопасность'} #${v.id} · ${money(v.salaryPerHour)}/ч`),button('fire-'+v.id,'Уволить','fireEmployee',[v.id]))),
          button('overwork',g.team.overwork?'Отменить переработки':'Включить переработки','toggleOverwork')),
        card('technology',head('tech-title','Технологии'),...cfg.TECH_NODES.map(v=>button('tech-'+v.id,`${v.name} · ${money(v.cost)} · IQ ${v.iqThreshold}`,'unlockTech',[v.id],!m.state.tech.includes(v.id))),
          button('license',m.state.licensed?'Лицензирование включено':`Лицензирование · IQ ${cfg.LICENSE_IQ_THRESHOLD}`,'toggleLicense'),
          row('opensource',button('open-source','Открыть исходный код','chooseOpenSource',[true],!m.state.openSourceChosen&&m.state.iq>=cfg.OPEN_SOURCE_IQ_THRESHOLD),button('keep-closed','Оставить модель закрытой','chooseOpenSource',[false],!m.state.openSourceChosen&&m.state.iq>=cfg.OPEN_SOURCE_IQ_THRESHOLD))))
      const panels=content.children!
      content=column('training',panels[0],{kind:'columns',id:'training-dashboard',children:[
        column('training-main',panels[4],panels[2],panels[3]),column('training-side',panels[1],panels[5])
      ]},panels[6])
    } else if (ui.page==='testing') {
      mode='panel';const b=m.benchmark.last
      content=column('testing',modelTitle,
        card('benchmark',head('gmi-title','Тестирование GMI'),text('gmi-cost',`${money(cfg.BENCHMARK_COST)} · Офлайн на ${cfg.BENCHMARK_OFFLINE_HOURS} игровых часов`),
          button('run-benchmark',m.benchmark.testing?'Тест выполняется':'Запустить тест','runBenchmark',[],!m.benchmark.testing,'primary'),
          button('preparing',m.benchmark.preparing?'Отключить подготовку к тесту':'Подготовиться к тесту','togglePreparing'),
          text('benchmark-risk','Подготовка повышает результат теста, но снижает токен-доход и создаёт риск разоблачения.'),
          b?profile('benchmark-scores',b):text('no-benchmark','Результатов пока нет.'),
          text('benchmark-date',b?`День ${b.day} · ${benchmarkIsCurrent(m)?'Текущая версия':'Устаревшая версия'}${b.exposed?' · Накрутка раскрыта':''}`:'')),
        card('market',head('market-title','Рынок'),text('market-users',`Пользователи ${n(m.users)} / вместимость ${n(rates.capacity)}`),
          button('advertise',`${g.market.advertising?'Отключить':'Включить'} рекламу · ${money(cfg.AD_DAILY_COST)}/день`,'toggleAdvertising'),
          slider('token-price',`Цена токенов ${g.market.tokenPrice}%`,g.market.tokenPrice,cfg.TOKEN_PRICE_MIN,cfg.TOKEN_PRICE_MAX,5,'setTokenPrice'),
          button('insurance',`${g.market.insurance?'Отключить':'Включить'} страховку · ${money(cfg.INSURANCE_DAILY_PREMIUM)}/день`,'toggleInsurance')),
        card('competitor',head('competitor-title','Конкурент'),text('competitor-score',`GMI ${g.competitor.revealedUntil!==null&&g.elapsedGameHours<g.competitor.revealedUntil?n(g.competitor.score,1):'≈ '+n(Math.round(g.competitor.score/10)*10)}`),
          {kind:'chart',id:'competitor-history',samples:g.competitor.samples,label:'История GMI конкурента'},button('espionage',`Промышленный шпионаж · ${money(cfg.ESPIONAGE_COST)}`,'attemptEspionage')),
        card('regions',head('regions-title','Новые регионы'),...cfg.REGIONS.map(v=>column('region-'+v.id,text('region-name-'+v.id,v.name+' · '+v.description),button('unlock-'+v.id,`Открыть · ${money(v.unlockCost)}`,'unlockRegion',[v.id],!g.regions.includes(v.id)),...v.locations.map(l=>button('region-location-'+l.id,l.name+' · '+money(l.price),'purchaseLocation',[l.id],g.regions.includes(v.id)&&!g.regionLocations.find(x=>x.id===l.id)?.owned))))))
    } else if (ui.page==='catalog') {
      mode='panel';content=column('catalog',head('catalog-title','Каталог моделей'),text('catalog-lead',c.strategy==='flagship'?'Новая база необратимо заменяет флагман. Прогресс, очередь и аудитория не переносятся.':'До четырёх независимых моделей. Новая модель получает нулевую квоту мощности.'),
        ...BASE_MODELS.map(b=>card('base-'+b.id,head('base-title-'+b.id,b.name),text('base-spec-'+b.id,`${b.parametersB}B · ${n(catalogWeightGb(b.id),1)} GB FP16 · Инференс ×${b.inferenceCost} · Обучение ×${b.trainingCost}`),profile('base-gmi-'+b.id,b.gmi),
          button('buy-base-'+b.id,`${money(b.licensePrice)} · ${c.purchasedBases.includes(b.id)?'Лицензия куплена':'Выбрать базу'}`,'ui:base',[b.id],!c.purchasedBases.includes(b.id)))))
    } else if (ui.page==='portfolio') {
      mode='panel';content=column('portfolio',head('portfolio-title',c.strategy==='portfolio'?'Портфель моделей':'Флагман'),
        text('compute-budget',`Мощность ${n(e.effectiveCompute,2)} · Свободная квота ${n(remainingAllocationBps(c)/100)}%`),
        ...c.models.map(v=>{const r=e.models.find(x=>x.modelId===v.id)!; return card('portfolio-'+v.id,
          head('portfolio-title-'+v.id,modelDisplayName(v)),text('portfolio-base-'+v.id,baseCatalogLabel(v)+' · '+modelIdleStatus(c,v).label),profile('portfolio-gmi-'+v.id,effectiveProfile(v)),
          text('portfolio-economy-'+v.id,`${n(r.allocatedCompute,2)} compute · ${n(v.users)} пользователей · ${money(r.revenuePerHour)}/ч`),
          slider('allocation-'+v.id,`Квота ${v.allocationBps/100}%`,v.allocationBps/100,0,Math.min(100,(remainingAllocationBps(c,v.id))/100),1,'ui:allocation',[v.id]),
          row('portfolio-actions-'+v.id,button('select-'+v.id,'Выбрать','selectModel',[v.id]),button('quant-'+v.id,QUANTIZATION_LABELS[v.quantization]+' · Квантование','ui:quant',[v.id]),button('rename-'+v.id,'Название','ui:rename',[v.id]))) }))
    } else if (ui.page==='interior') {
      mode='panel';const loc=normalizeLocation([...g.locations,...g.regionLocations].find(v=>v.id===ui.location)??g.locations[0]); const d=locationDefinition(loc.id), le=portfolioLocationEconomy(c,loc.id)
      const cells:Node[]=[]
      for(let r=0;r<loc.gridSize.rows;r++) for(let col=0;col<loc.gridSize.cols;col++) {
        const cell={row:r,col}, server=loc.installedServers.find(v=>sameCell(v.gridPosition,cell)),rig=loc.rigs?.find(v=>sameCell(v.gridPosition,cell))
        cells.push(button(`cell-${r}-${col}`,`${r+1}:${col+1} ${server?cfg.CHIPS[server.chip].name:rig?cfg.CHASSIS[rig.chassis].name:'Свободно'}`,'ui:cell',[loc.id,r,col],loc.owned,ui.cell&&sameCell(ui.cell,cell)?'selected':'secondary'))
      }
      const server=loc.installedServers.find(v=>v.id===ui.selectedServer)
      content=column('interior',head('interior-title',d.name),text('interior-rates',`${n(le.effectiveCompute,2)} compute · ${n(le.demandKw,2)} / ${d.powerLimitKw} кВт · ${money(le.profitPerHour)}/ч`),
        row('interior-actions',button('walk-in','Войти в помещение','host:enter',[loc.id],loc.owned),button('procurement','Закупки и склад','ui:procurement',[loc.id]),page('map','Город')),
        {kind:'grid',id:'server-grid',columns:loc.gridSize.cols,children:cells},
        ...(server?[card('selected-server',head('server-title',cfg.CHIPS[server.chip].name),text('server-output',`${n(serverOutput(server).compute,2)} compute · ${n(serverOutput(server).powerKw,2)} кВт`),
          slider('overclock',`Разгон ${n(server.overclock*100)}%`,server.overclock*100,50,150,5,'ui:overclock',[loc.id,server.id]),
          row('server-actions',button('upgrade','Апгрейд','ui:procurement',[loc.id]),button('sell','Продать чип','sellAt',[loc.id,server.id])))]:[]),
        ...loc.installedServers.filter(v=>!v.gridPosition).map(v=>row('reserve-'+v.id,text('reserve-name-'+v.id,'Резерв: '+cfg.CHIPS[v.chip].name),button('deploy-'+v.id,'Установить в свободную ячейку','deployReserve',[loc.id,v.id,firstFreeCell(loc)],!!firstFreeCell(loc)),button('reserve-sell-'+v.id,'Продать','sellAt',[loc.id,v.id]))))
    } else if (ui.page==='office') {
      mode='panel'; content=column('office',head('office-title','Офис компании'),text('office-cash',money(g.cash)),text('office-portfolio',`${c.models.length} моделей · ${n(companyUsers(c))} пользователей`),text('office-owned',`${g.locations.filter(v=>v.owned).length} городских площадок`),button('office-models','Модели','ui:page',['portfolio']),button('office-enter','Войти в офис','host:office',[],g.officeOwned),button('office-buy',`Купить офис · ${money(OFFICE_PRICE)}`,'purchaseOffice',[],!g.officeOwned))
    } else {
      // World remains readable. Location browser is a deliberate drawer, never a permanent wall.
      const loc=[...g.locations,...g.regionLocations].find(v=>v.id===ui.location)??g.locations[0],d=locationDefinition(loc.id)
      const size=normalizeLocation(loc).gridSize
      content=column('location-card',text('location-eyebrow',loc.owned?'Ваша серверная':'Будущая серверная','muted'),head('location-name',d.name),text('location-description',loc.owned?'Площадка куплена. Расставляйте серверы в ячейках внутри помещения.':d.description),
        ...(!loc.owned?[text('location-grid',`Сетка помещения     ${size.rows} × ${size.cols}`),text('location-power',`Энергосеть     ${d.powerLimitKw} кВт`),text('location-rent',`Аренда     ${n(d.rentPerHour)} $/ч`),button('buy-location',`Купить локацию    ${n(d.price)} $`,'purchaseLocation',[loc.id],true,'primary')]:
          [button('enter-location','Войти в интерьер','host:enter',[loc.id],true,'primary'),button('open-location','Оборудование','ui:interior',[loc.id])]),
        button('locations-drawer','Все локации','ui:modal',['locations']))
      if(host.walking) content=column('walk-prompt',text('walk-hint',host.inputHint||'WASD — движение · ПКМ — осмотр · E — взаимодействие'),button('leave-interior','Вернуться в город','host:leave'),button('walk-operations','Управление площадкой','ui:interior',[ui.location]))
    }
  }
  const toolbar: Node|null=s.phase==='playing'&&!g.ending?column('toolbar',{kind:'bar',id:'status-bar',children:[
    button('brand-button','Neuron.','ui:page',['map'],true,'brand'),
    column('capital-metric',text('capital-label','Капитал','muted'),button('cash',n(g.cash)+' $','ui:modal',['economy'],true,'metric')),
    column('profit-metric',text('profit-label','Прибыль в час','muted'),text('profit',n(e.profitPerHour)+' $',e.profitPerHour<0?'danger':'positive')),
    {kind:'spacer',id:'toolbar-space'},
    text('clock',`День ${Math.floor((g.elapsedGameHours+8)/24)+1}  ${n(Math.floor((g.elapsedGameHours+8)%24)).padStart(2,'0')}:${n(Math.floor((g.elapsedGameHours%1)*60)).padStart(2,'0')}`),
    iconButton(button('pause',g.paused?'Продолжить':'Пауза','togglePause'),g.paused?'play':'pause'),
    button('speed-1','1×','setSpeed',[1],true,g.speed===1?'selected':'secondary'),button('speed-3','3×','setSpeed',[3],true,g.speed===3?'selected':'secondary'),
    iconButton(button('save','Сохранить','host:save'),'save'),iconButton(button('settings','Настройки','ui:modal',['settings']),'settings'),
  ]},row('navigation',...[
    page('map','Город'),iconButton(page('training','Обучение'),'model'),iconButton(page('testing','GMI и рынок'),'flask'),iconButton(page('catalog','Каталог'),'wallet'),
    iconButton(page('portfolio','Модели'),'server'),page('office','Офис'),iconButton(button('events','События','ui:modal',['events']),'clock'),
    iconButton(button('toolbar-help','Помощь','ui:modal',['help']),'help')
  ].map(v=>({...v,role:v.id==='nav-'+ui.page?'selected':v.role})))):null
  if(ui.modal==='locations') modal=column('locations',head('locations-title','Город и недвижимость'),...g.locations.map(l=>button('goto-'+l.id,locationDefinition(l.id).name+(l.owned?' · В собственности':' · '+money(locationDefinition(l.id).price)),'ui:location',[l.id])),...g.regionLocations.filter(l=>l.owned).map(l=>button('goto-'+l.id,locationDefinition(l.id).name,'ui:location',[l.id])),
    ...CITY_TOWERS.map(t=>card('tower-'+t.id,head('tower-title-'+t.id,t.name),text('tower-description-'+t.id,t.description),text('tower-rent-'+t.id,`Аренда ${money(t.rentPerHour)}/ч · Обслуживание ${money(t.upkeepPerHour)}/ч`),button('tower-buy-'+t.id,money(t.price),'purchaseCityTower',[t.id],!(g.cityProperties??[]).includes(t.id)))),
    ...CHIP_SHOPS.map(t=>button('shop-'+t.id,t.name,'ui:procurement',[ui.location])),button('auction','Аукцион','host:auction'),button('nuclear','АЭС','host:nuclear'),button('greenhaven','Greenhaven','host:greenhaven'),close)
  if(ui.modal==='economy') modal=column('economy',head('economy-title','Финансы компании'),...[
    ['Баланс',g.cash],['Доход моделей / ч',e.serverRevenuePerHour],['Арендный доход / ч',e.propertyRevenuePerHour],['Выручка / ч',e.revenuePerHour],['Расходы / ч',e.expensesPerHour],['Прибыль / ч',e.profitPerHour],['Капитальные вложения',g.totalCapex],['Выручка за всё время',g.totalRevenue],['Расходы за всё время',g.totalExpenses],
  ].map(([label,value],i)=>row('ledger-'+i,text('ledger-label-'+i,String(label)),text('ledger-value-'+i,money(Number(value))))),text('economy-compute',`Мощность ${n(e.effectiveCompute,2)} · Репутация ${n(g.reputation)}`),...g.locations.filter(l=>l.owned).map(l=>{const x=portfolioLocationEconomy(c,l.id);return text('location-profit-'+l.id,`${locationDefinition(l.id).name} · ${money(x.expensesPerHour)}/ч расходов · ${n(x.effectiveCompute,2)} compute`)}),close)
  if(ui.modal==='events') modal=column('events',head('events-title','Журнал событий'),...s.eventLog.slice().reverse().map((v,i)=>text('event-'+i,`День ${Math.floor(v.atHours/24)+1} · ${v.message}`,v.kind==='error'?'danger':'body')),close)
  if(ui.modal==='settings') modal=column('settings',head('settings-title','Настройки'),text('settings-info','Настройки сохраняются отдельно от партии.'),
    slider('master-volume','Общая громкость',Number(ui.fields.volume??'80'),0,100,5,'host:volume'),
    slider('text-scale','Размер интерфейса',Number(ui.fields.scale??'100'),85,140,5,'host:scale'),
    row('settings-actions',button('fullscreen','Полноэкранный режим','host:fullscreen'),button('daynight','День / ночь','host:daynight'),button('help','Управление','ui:modal',['help'])),
    row('save-actions',button('manual-save','Сохранить','host:save',[],s.phase==='playing'),button('restore','Загрузить сохранение','ui:modal',['restore'],!!host.hasSave),button('reset','Начать заново','ui:modal',['reset'])),
    button('main-menu','Главное меню','host:menu'),button('quit-game','Выйти из игры','host:quit'),close)
  if(ui.modal==='help') modal=column('help',head('help-title','Управление'),text('help-map','Город: WASD — перемещение камеры; колесо — масштаб. Выберите площадку, купите её и закажите оборудование.'),text('help-interior','Помещение: WASD — ходьба, мышь — обзор, E — взаимодействие. Поставки требуют игрового времени; полученное оборудование устанавливается со склада без повторной оплаты.'),text('help-models','Каталог содержит готовые базы; их стартовая компетенция не является заработанным IQ. В портфеле распределите общую мощность между моделями. Проверка данных и обучение доступны у рабочего места.'),close)
  if(ui.modal==='reset'||ui.modal==='restore') modal=column('confirm',head('confirm-title',ui.modal==='reset'?'Новая партия?':'Загрузить сохранение?'),text('confirm-warning','Несохранённые изменения текущей партии будут потеряны.'),row('confirm-actions',close,button('confirm-operation','Продолжить',ui.modal==='reset'?'beginSetup':'host:load',[],true,'danger')))
  if(ui.modal==='base') {const b=BASE_MODELS.find(v=>v.id===ui.fields.base)!;modal=column('buy-base',head('buy-base-title',b.name+' · '+money(b.licensePrice)),input('baseName','Название модели',ui.fields.baseName),...flagshipReplaceBlockers(c).map((x,i)=>text('base-block-'+i,x,'danger')),text('base-confirm-warning',c.strategy==='flagship'?'Подтверждение необратимо заменит текущий флагман.':'После покупки назначьте модели мощность.'),row('base-confirm-actions',close,button('confirm-base','Подтвердить покупку','ui:purchase-base',[],ui.fields.baseName.trim().length>0&&ui.fields.baseName.trim().length<=48,'primary')))}
  if(ui.modal==='rename') modal=column('rename-model',head('rename-title','Название модели'),input('rename','Название',ui.fields.rename),row('rename-actions',close,button('confirm-rename','Сохранить','ui:rename-commit')))
  if(ui.modal==='quant') {const step=Number(ui.fields.quant) as QuantizationStep,p=quantizationPreview(m,step);modal=column('quantization',head('quant-title','Квантование · '+modelDisplayName(m)),select('quant','Точность',String(step),Object.entries(QUANTIZATION_LABELS).map(([value,label])=>({value,label}))),profile('quant-preview',p.gmi),text('quant-quality',`Качество ×${n(p.quality,4)} · Пропускная способность ×${n(p.throughput,4)} · Эффективный IQ ${n(p.learnedIq,2)}`),text('quant-warning','Квантование обратимо; полный прогресс обучения сохраняется. Результат GMI стареет при изменении точности.'),row('quant-actions',close,button('apply-quant','Применить','setModelQuantization',[m.id,step],!m.state.run&&!m.benchmark.testing,'primary')))}
  if(ui.modal==='procurement') modal=procurement(s,ui,close)
  if(s.phase==='playing'&&!g.ending) {
    const pending=g.contracts.pending
    if(pending&&!ui.modal) modal=column('contract',head('contract-client',pending.clientName),text('contract-description',`Предложение действует до дня ${pending.expiresDay}. Официальный контракт требует чистых данных и соблюдения условий.`),
      select('contractModel','Исполнитель',ui.fields.contractModel??m.id,c.models.map(v=>({value:v.id,label:modelDisplayName(v)}))),
      row('contract-options',...(['official','grey',...(pending.enterprise?['enterprise']:[])] as const).map(k=>button('contract-'+k,`${k==='official'?'Официальный':k==='grey'?'Серый':'Корпоративный'} · ${money(k==='official'?pending.officialPayout:k==='grey'?pending.greyPayout:pending.enterprisePayout)}`,'ui:contract',[k])),button('decline-contract','Отказаться','declineContract')))
    if(!m.state.personality&&!ui.modal) modal=column('personality',head('personality-title','Характер модели'),text('personality-description','Выбор влияет на удержание аудитории и доход от токенов.'),button('friendly','Дружелюбная','setPersonality',['friendly']),button('raw','Без фильтра','setPersonality',['raw']))
    if(g.acquisitionOffered&&!g.acquisitionDeclined&&!ui.modal) modal=column('acquisition',head('acquisition-title','Предложение о продаже компании'),text('acquisition-price',money(cfg.ACQUISITION_OFFER)),text('acquisition-warning','Продажа завершает партию. Текущие показатели войдут в итоговый отчёт.'),button('accept-acquisition','Принять предложение','acceptAcquisition'),button('decline-acquisition','Продолжить самостоятельно','declineAcquisition'))
  }
  if(modal) modal={...modal,id:'modal-'+modal.id}
  return {schema:1,mode,page:ui.page,content,toolbar,modal,notice:host.saveError||s.notice?.message||null,noticeKind:s.notice?.kind??'error'}
}

function procurement(s:Store,ui:UiState,close:Node):Node {
  const loc=normalizeLocation([...s.company.company.locations,...s.company.company.regionLocations].find(v=>v.id===ui.location)??s.company.company.locations[0])
  const server=loc.installedServers.find(v=>v.id===ui.selectedServer), rig=loc.rigs?.find(v=>sameCell(v.gridPosition,ui.cell))
  const chassis=server?.chassis??rig?.chassis??ui.chassis, chips=Object.values(cfg.CHIPS).filter(c=>chassisSupports(chassis,c.id)&&(!server||c.compute>cfg.CHIPS[server.chip].compute))
  const chip=chips.find(c=>c.id===ui.chip)??chips[0]
  const kind=server||rig?'chip':ui.orderKind
  const qty=ui.qty, channel=ui.channel
  const unit=kind==='chassis'?cfg.CHASSIS[chassis].price:chip?.price??0
  const price=orderPrice(unit,channel,qty)+(kind==='kit'?orderPrice(cfg.CHASSIS[chassis].price,channel,qty):0)
  const inv=loc.inventory??{chips:{},chassis:{}}
  return column('procurement',head('procurement-title','Закупки · '+locationDefinition(loc.id).name),
    row('purchase-types',button('mode-kit','Комплект','ui:kind',['kit'],!server&&!rig),button('mode-chip','Чип','ui:kind',['chip']),button('mode-chassis','Стойка','ui:kind',['chassis'],!server&&!rig)),
    select('chassis','Стойка',chassis,Object.values(cfg.CHASSIS).map(v=>({value:v.id,label:v.name}))),
    select('chip','Совместимый чип',chip?.id??'',chips.map(v=>({value:v.id,label:`${v.name} · ${v.compute} compute · ${v.powerKw} кВт`}))),
    select('channel','Канал поставки',channel,[{value:'official',label:'Официальный'},{value:'grey',label:'Серый импорт · риск брака'}]),
    slider('quantity','Количество',qty,1,kind==='kit'?1:cfg.MAX_ORDER_QTY,1,'ui:quantity'),
    text('purchase-price',`Итого ${money(price)} · ${kind==='chassis'?deliveryHours('chassis',chassis,channel):chip?deliveryHours('chip',chip.id,channel):0} игровых ч.`),
    button('place-order','Оформить заказ','ui:order',[],loc.owned&&(kind==='chassis'||!!chip),'primary'),
    text('delivery-info','Деньги списываются при заказе. После доставки монтаж использует складские запасы.'),
    ...s.company.company.orders.filter(o=>o.locationId===loc.id).map(o=>text('order-'+o.id,`Заказ #${o.id}: ${o.qty} × ${o.item} · осталось ${n(Math.max(0,o.arriveAt-s.company.company.elapsedGameHours),1)} ч.`)),
    ...(s.company.company.paused?[button('resume-deliveries','Продолжить время','togglePause')]:[]),
    head('stock-title','Склад площадки'),
    ...Object.entries(inv.chassis).filter(([,count])=>Number(count)>0).map(([id,count])=>row('stock-'+id,text('stock-count-'+id,`${cfg.CHASSIS[id as ChassisId].name} × ${count}`),button('mount-'+id,'Установить стойку','mountChassis',[loc.id,ui.cell??firstFreeCell(loc),id],!!(ui.cell??firstFreeCell(loc))))),
    ...Object.entries(inv.chips).filter(([,count])=>Number(count)>0).map(([id,count])=>row('stock-'+id,text('stock-count-'+id,`${cfg.CHIPS[id as ChipId].name} × ${count}`),button('mount-'+id,'Установить / заменить','mountChip',[loc.id,ui.cell,id],!!ui.cell&&!!(server||rig)))),close)
}

/** Resolve form actions against the latest committed input, not captured render-time values. */
export function uiCommand(ui:UiState,action:string,raw:unknown,s:Store):{ui:UiState;action?:string;args?:unknown[]} {
  if(!Array.isArray(raw)||raw.length>8) throw new Error('Invalid UI action')
  const a=raw, next={...ui,fields:{...ui.fields}}
  const result=(command?:string,args?:unknown[])=>({ui:next,action:command,args})
  if(action==='page') {if(!['map','interior','training','testing','catalog','portfolio','office'].includes(String(a[0])))throw new Error('Unknown page');next.page=String(a[0]);next.modal='';return result()}
  if(action==='modal') {if(!['','locations','economy','events','settings','help','restore','reset','procurement','base','quant','rename'].includes(String(a[0])))throw new Error('Unknown dialog');next.modal=String(a[0]);return result()}
  if(action==='field') {
    if(typeof a[0]!=='string'||typeof a[1]!=='string'||a[1].length>512)throw new Error('Invalid input')
    const [key,value]=a as string[]
    if (key==='base' && !BASE_MODELS.some(b=>b.id===value)) throw new Error('Unknown base')
    if (key==='strategy' && !['flagship','portfolio'].includes(value)) throw new Error('Unknown strategy')
    if (key==='quant' && !Object.hasOwn(QUANTIZATION_LABELS,value)) throw new Error('Invalid quantization')
    if (['volume','scale'].includes(key) && (!Number.isFinite(Number(value)) || Number(value)<0 || Number(value)>200)) throw new Error('Invalid setting')
    if(['name','rename','baseName','strategy','quant','contractModel','base','companyName','volume','scale'].includes(key))next.fields[key]=value
    else if(key==='domain'&&Object.hasOwn(DOMAIN_LABELS,value))next.domain=value as DataDomain
    else if(key==='chip'&&Object.hasOwn(cfg.CHIPS,value))next.chip=value as ChipId
    else if(key==='chassis'&&Object.hasOwn(cfg.CHASSIS,value))next.chassis=value as ChassisId
    else if(key==='channel'&&['official','grey'].includes(value))next.channel=value as Channel
    else throw new Error('Unknown input')
    if(key==='quant'&&!['0','1','2'].includes(value))throw new Error('Invalid quantization')
    if(key==='strategy'&&!['flagship','portfolio'].includes(value))throw new Error('Invalid strategy')
    return result()
  }
  if(action==='location'||action==='interior'||action==='procurement'||action==='cell') {
    const loc=[...s.company.company.locations,...s.company.company.regionLocations].find(v=>v.id===a[0]);if(!loc)throw new Error('Unknown location')
    next.location=loc.id;next.modal=action==='procurement'?'procurement':'';next.page=action==='interior'||action==='cell'?'interior':action==='location'?'map':ui.page
    if(action==='cell') {
      const normalized=normalizeLocation(loc),r=Number(a[1]),c=Number(a[2]);if(!Number.isInteger(r)||!Number.isInteger(c)||r<0||c<0||r>=normalized.gridSize.rows||c>=normalized.gridSize.cols)throw new Error('Invalid cell')
      next.cell={row:r,col:c};next.selectedServer=normalized.installedServers.find(v=>sameCell(v.gridPosition,next.cell))?.id??null
      if(!next.selectedServer)next.modal='procurement'
    } else if(ui.location!==loc.id) {next.cell=null;next.selectedServer=null}
    return result()
  }
  if(action==='kind') {if(!['kit','chip','chassis'].includes(String(a[0])))throw new Error('Invalid order kind');next.orderKind=a[0] as UiState['orderKind'];if(next.orderKind==='kit')next.qty=1;return result()}
  if(action==='quantity') {const q=Number(a[0]);if(!Number.isInteger(q)||q<1||q>cfg.MAX_ORDER_QTY)throw new Error('Invalid quantity');next.qty=q;return result()}
  if(action==='start')return result('startNewGame',[ui.fields.strategy,ui.fields.name])
  if(action==='base') {if(!BASE_MODELS.some(v=>v.id===a[0]))throw new Error('Unknown base');next.fields.base=String(a[0]);next.fields.baseName='';next.modal='base';return result()}
  if(action==='purchase-base')return result('purchaseBase',[ui.fields.base,ui.fields.baseName,s.company.strategy==='flagship'])
  if(action==='quant'||action==='rename') {const m=s.company.models.find(v=>v.id===a[0]);if(!m)throw new Error('Unknown model');next.modal=action;next.fields.quant=String(m.quantization);next.fields.rename=modelDisplayName(m);return result('selectModel',[m.id])}
  if(action==='rename-commit')return result('renameSelected',[ui.fields.rename])
  if(action==='contract')return result('acceptContract',[a[0],ui.fields.contractModel??s.selectedModelId])
  if(action==='overclock')return result('overclockAt',[a[0],a[1],Number(a[2])/100])
  if(action==='allocation') {if(s.company.strategy!=='portfolio')throw new Error('Флагман использует всю мощность.');const allocations=Object.fromEntries(s.company.models.map(v=>[v.id,v.id===a[0]?Math.round(Number(a[1])*100):v.allocationBps]));return result('setAllocations',[allocations])}
  if(action==='order') {
    const loc=normalizeLocation([...s.company.company.locations,...s.company.company.regionLocations].find(v=>v.id===ui.location)!)
    const server=loc.installedServers.find(v=>v.id===ui.selectedServer),rig=loc.rigs?.find(v=>sameCell(v.gridPosition,ui.cell))
    const chassis=server?.chassis??rig?.chassis??ui.chassis, chips=Object.values(cfg.CHIPS).filter(c=>chassisSupports(chassis,c.id)&&(!server||c.compute>cfg.CHIPS[server.chip].compute)),chip=chips.find(v=>v.id===ui.chip)??chips[0]
    const kind=server||rig?'chip':ui.orderKind
    if(kind==='kit')return result('orderKit',[{locationId:loc.id,chassis,chip:chip?.id,channel:ui.channel,qty:1,targetCell:ui.cell??firstFreeCell(loc)}])
    return result('orderEquipment',[{locationId:loc.id,kind,item:kind==='chassis'?chassis:chip?.id,channel:ui.channel,qty:ui.qty,targetCell:ui.cell,targetServerId:server?.id??null}])
  }
  throw new Error('Unknown UI action')
}
