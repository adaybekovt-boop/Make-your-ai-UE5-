import {build} from 'esbuild'
import fs from 'node:fs'
import path from 'node:path'
import vm from 'node:vm'
import assert from 'node:assert/strict'
import {spawn} from 'node:child_process'
import {createInterface} from 'node:readline'
import {fileURLToPath} from 'node:url'
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..')
const host=process.env.MAI_RULES_HOST
if(!host)throw new Error('Set MAI_RULES_HOST to the actually compiled rules_host executable')
const rules=path.join(root,'Unreal/Rules')
const code=await build({entryPoints:[path.join(root,'Unreal/Tests/browser_rules_oracle.ts')],bundle:true,write:false,platform:'neutral',format:'iife',target:'es2022',define:{'import.meta.env.DEV':'false'},plugins:[{name:'oracle-headless-binding-only',setup(b){
  b.onResolve({filter:/^zustand$/},()=>({path:path.join(rules,'vanilla.ts')}))
  b.onResolve({filter:/^idb$/},()=>({path:path.join(rules,'noBrowserStorage.ts')}))
  b.onResolve({filter:/^\.\.\/persistence\/companySaves$/},a=>a.importer.replaceAll('\\','/').endsWith('/src/store/gameStore.ts')?{path:path.join(rules,'nativePersistence.ts')}:undefined)
}}]})
let assertions=0,scenarios=0,traceSteps=0
function near(a,b,where='company'){
  assertions++
  if(typeof a==='number'&&typeof b==='number'){assert(Number.isFinite(a)&&Number.isFinite(b),where);assert(Math.abs(a-b)<=1e-8+Math.abs(b)*1e-11,`${where}: ${a} != ${b}`);return}
  if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);for(const k of Object.keys(a))near(a[k],b[k],where+'.'+k);return}
  assert.deepEqual(a,b,where)
}
function native(){const process=spawn(host,[path.join(root,'Unreal/MakeYourAI/Content/Rules/mai-rules.js')],{stdio:['pipe','pipe','inherit']});const lines=createInterface({input:process.stdout});let waiting=[];lines.on('line',s=>{const item=waiting.shift();if(!item)throw new Error('Unexpected native response');item.resolve(JSON.parse(s))});process.on('exit',code=>{for(const item of waiting)item.reject(new Error('Native VM exited '+code));waiting=[]});return {call:r=>new Promise((resolve,reject)=>{waiting.push({resolve,reject});process.stdin.write(JSON.stringify(r)+'\n')}),close:()=>{lines.close();process.stdin.end()},process}}
const cmd=(action,...args)=>({method:'command',action,args})
async function differential(strategy){
  const context=vm.createContext({console,structuredClone});new vm.Script(code.outputFiles[0].text).runInContext(context);await context.Oracle.boot();const n=native()
  const trace=[cmd('beginSetup'),cmd('startNewGame',strategy,'Parity Model'),cmd('purchaseLocation','garage'),cmd('purchaseLocation','garage'),
    cmd('purchaseLocation','dc-north'),cmd('orderKit',{locationId:'garage',chassis:'rack-basic',chip:'flagship',channel:'official',qty:1,targetCell:{row:0,col:0}}),
    cmd('orderKit',{locationId:'garage',chassis:'rack-basic',chip:'consumer-gpu',channel:'official',qty:1,targetCell:{row:0,col:0}}),cmd('mountChassis','garage',{row:0,col:0},'rack-basic')]
  // 1x is the primary timing path. 3x is separately compared, not substituted.
  trace.push(...Array.from({length:1441},()=>({method:'tick',seconds:.25})))
  trace.push(cmd('mountChassis','garage',{row:0,col:0},'rack-basic'),cmd('mountChip','garage',{row:0,col:0},'consumer-gpu'),cmd('setPersonality','friendly'),cmd('buyDataLot','unofficial','general'))
  for(const r of trace){const expected=await context.Oracle.execute(r),actual=await n.call(r);assert(actual.value,'No native state for '+JSON.stringify(r));near(actual.value.company,JSON.parse(JSON.stringify(expected)));traceSteps++}
  let state=(await n.call({method:'state'})).value
  assert(state.company.company.locations.find(l=>l.id==='garage').installedServers.length===1,'Kit must actually be delivered and mounted')
  assert(state.company.models[0].state.queue.length>0,'Dataset purchase did not happen')
  const block=await n.call(cmd('startTraining'));assert.equal(block.ok,false);assert.match(block.error,/Unreviewed/);scenarios++
  await n.call({method:'sync-review',verified:state.company.models[0].state.queue.map(l=>state.company.models[0].id+':'+l.id),reservedCompute:0})
  for(const r of [cmd('startTraining'),...Array.from({length:200},()=>({method:'tick',seconds:.25})),cmd('togglePause'),{method:'tick',seconds:1},cmd('togglePause'),cmd('setSpeed',3),...Array.from({length:50},()=>({method:'tick',seconds:.25})),cmd('setSpeed',1)]){
    const expected=await context.Oracle.execute(r),actual=await n.call(r);assert(actual.value,JSON.stringify(actual));near(actual.value.company,JSON.parse(JSON.stringify(expected)));traceSteps++
    if(r.action==='startTraining'){
      assert(actual.value.company.models[0].state.run,'Training must really start before testing its progress UI')
      await n.call(cmd('ui:page','training'))
      const view=(await n.call({method:'view'})).value.content
      const find=(node,id)=>node.id===id?node:(node.children??[]).map(child=>find(child,id)).find(Boolean)
      assert.equal(find(view,'training-progress').kind,'ring');assert.equal(find(view,'training-progress').value,0)
      assert.equal(find(view,'train').enabled,false)
      await n.call(cmd('ui:page','map'))
    }
  }
  // VM process boundary, restored RNG and original V3 codec: not a UE UI playthrough.
  await n.call(cmd('ui:field','domain','coding'))
  const saved=(await n.call({method:'save',savedAt:'2026-09-08T00:00:00Z'})).value
  const second=native();assert.equal((await second.call({method:'load',payload:saved})).ok,true)
  assert.equal((await second.call({method:'state'})).value.ui.domain,'coding')
  const badDomain=structuredClone(saved);badDomain.ui.domain='unknown-domain'
  assert.equal((await second.call({method:'validate',payload:badDomain})).ok,false)
  assert.equal((await second.call({method:'load',payload:badDomain})).ok,false)
  assert.equal((await second.call({method:'state'})).value.ui.domain,'coding')
  for(let i=0;i<20;i++){const r={method:'tick',seconds:.25};near((await n.call(r)).value.company,(await second.call(r)).value.company)}
  const before=(await second.call({method:'state'})).value.company
  const corrupt=structuredClone(saved);corrupt.browser.game.company.cash='broken';assert.equal((await second.call({method:'load',payload:corrupt})).ok,false);near((await second.call({method:'state'})).value.company,before)
  const unknown=await second.call(cmd('eval','require("fs")'));assert.equal(unknown.ok,false);near((await second.call({method:'state'})).value.company,before)
  const unknownBase=structuredClone(saved);unknownBase.ui.fields.base='not-a-base';assert.equal((await second.call({method:'validate',payload:unknownBase})).ok,false)
  const viewBefore=(await second.call({method:'state'})).value
  for(const location of ['garage','campus','dc-north','dc-south']){
    await second.call(cmd('ui:location',location))
    assert.equal((await second.call({method:'view'})).value.selectedLocation,location,'Map marker selection must reflect canonical UI location')
  }
  await second.call(cmd('ui:location',viewBefore.ui.location))
  for(const page of ['map','training','testing','catalog','portfolio','office']){
    await second.call(cmd('ui:page',page));const v=await second.call({method:'view'});assert.equal(v.ok,true,JSON.stringify(v));const ids=new Set();function visit(x){if(!x)return;assert(!ids.has(x.id),'Duplicate UMG identity '+x.id);ids.add(x.id);for(const child of x.children??[])visit(child)};visit(v.value.toolbar);visit(v.value.content);visit(v.value.modal)
    assert(!JSON.stringify(v).match(/SOURCE_SCAFFOLD|portable vertical slice|NOT VERIFIED/))
    if(page==='training'){
      const nodes=new Map();const collect=x=>{nodes.set(x.id,x);for(const child of x.children??[])collect(child)};collect(v.value.content)
      const model=viewBefore.company.models.find(x=>x.id===viewBefore.selectedModelId)??viewBefore.company.models[0]
      const run=model.state.run,ring=nodes.get('training-progress')
      assert.equal(ring.kind,'ring');assert.equal(ring.value,run&&run.total>0?Math.max(0,Math.min(1,1-run.remaining/run.total)):-1)
      assert.equal(nodes.get('training-categories').columns,2);assert.equal(nodes.get('training-categories').children.length,4)
      assert.equal(nodes.get('domain').children.length,5)
      for(const domain of ['general','reasoning','coding','safety','multimodal']){
        const choice=nodes.get('domain-'+domain);assert.equal(choice.action,'ui:field');assert.deepEqual(choice.args,['domain',domain])
        assert.equal((await second.call(cmd(choice.action,...choice.args))).ok,true)
        const updated=(await second.call({method:'view'})).value.content
        const find=(node,id)=>node.id===id?node:(node.children??[]).map(child=>find(child,id)).find(Boolean)
        assert.equal(find(updated,'domain-'+domain).role,'selected')
        assert.deepEqual(find(updated,'official-data').args,['official',domain])
      }
      await second.call(cmd('ui:field','domain',viewBefore.ui.domain))
    }
  }
  for(const dialog of ['locations','economy','events','settings','help','restore','reset','procurement','base','quant','rename']){
    await second.call(cmd('ui:modal',dialog));const v=await second.call({method:'view'});assert.equal(v.ok,true,JSON.stringify(v));const ids=new Set();const visit=x=>{if(!x)return;assert(!ids.has(x.id),'Duplicate dialog identity '+x.id);ids.add(x.id);for(const child of x.children??[])visit(child)};visit(v.value.toolbar);visit(v.value.content);visit(v.value.modal)
  }
  const viewAfter=(await second.call({method:'state'})).value;near(viewBefore.company,viewAfter.company);assert.equal(viewBefore.rng,viewAfter.rng)
  n.close();second.close();scenarios+=11
}
await differential('flagship');await differential('portfolio')
const result={scope:'portable native VM versus pinned original browser store; NOT Unreal build or UI playthrough',strategies:2,scenarios,traceSteps,assertions,failures:0}
console.log(JSON.stringify(result,null,2))
if(process.env.MAI_RULES_REPORT)fs.writeFileSync(process.env.MAI_RULES_REPORT,JSON.stringify(result,null,2)+'\n')
