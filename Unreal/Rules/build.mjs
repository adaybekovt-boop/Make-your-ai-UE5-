import fs from 'node:fs'
import path from 'node:path'
import crypto from 'node:crypto'
import { fileURLToPath } from 'node:url'
import { build } from 'esbuild'
const here=path.dirname(fileURLToPath(import.meta.url)), root=path.resolve(here,'../..')
const output=path.join(root,'Unreal/MakeYourAI/Content/Rules')
fs.mkdirSync(output,{recursive:true})
const hash=data=>crypto.createHash('sha256').update(data).digest('hex')
const inputs={}
const reserveMarker='const infrastructure = calculateCompanyEconomy(companyView(state))'
let reservePatches=0
const result=await build({entryPoints:[path.join(here,'kernel.ts')],bundle:true,write:false,
  format:'iife',platform:'neutral',target:'es2022',treeShaking:true,metafile:true,minify:false,charset:'utf8',
  define:{'import.meta.env.DEV':'false'},
  banner:{js:fs.readFileSync(path.join(here,'prelude.js'),'utf8')},
  plugins:[{name:'native-only-adapters',setup(b){
    b.onResolve({filter:/^idb$/},()=>({path:path.join(here,'noBrowserStorage.ts')}))
    b.onResolve({filter:/^zustand$/},()=>({path:path.join(here,'vanilla.ts')}))
    b.onResolve({filter:/^\.\.\/persistence\/companySaves$/},args=>args.importer.replaceAll('\\','/').endsWith('/src/store/gameStore.ts')?{path:path.join(here,'nativePersistence.ts')}:undefined)
    b.onLoad({filter:/[\\/]src[\\/].*\.ts$/},args=>{
      const original=fs.readFileSync(args.path,'utf8'); inputs[path.relative(root,args.path).replaceAll('\\','/')]=hash(original)
      if(args.path.replaceAll('\\','/').endsWith('/src/systems/models/economy.ts')) {
        if(original.split(reserveMarker).length!==2)throw new Error('Browser economy changed: review compute reservation adapter')
        reservePatches++
        return {loader:'ts',contents:original.replace(reserveMarker,reserveMarker+`\n  // Native review extension shares compute; it never creates a second financial ledger.\n  const reserved = typeof globalThis.__maiReservedCompute === 'function' ? globalThis.__maiReservedCompute() : 0\n  infrastructure.effectiveCompute = Math.max(0, infrastructure.effectiveCompute - reserved)`)}
      }
      return {loader:'ts',contents:original}
    })
  }}]})
if(reservePatches!==1)throw new Error('Native compute adapter was not applied exactly once')
const dependencies=Object.keys(result.metafile.inputs)
if(dependencies.some(p=>/node_modules\/(react|react-dom|three|idb)\//.test(p.replaceAll('\\','/'))))throw new Error('Browser dependency leaked into native rules')
const bytes=result.outputFiles[0].contents
// Generated ownership check: do not erase a hand-edited bundle or an unowned file.
const bundlePath=path.join(output,'mai-rules.js'), manifestPath=path.join(output,'mai-rules.manifest.json')
if(fs.existsSync(bundlePath)) {
  if(!fs.existsSync(manifestPath))throw new Error('Existing rules bundle has no ownership manifest; retained')
  const previous=JSON.parse(fs.readFileSync(manifestPath,'utf8'))
  if(hash(fs.readFileSync(bundlePath))!==previous.bundleSha256)throw new Error('Existing rules bundle was edited; retained')
}
fs.writeFileSync(bundlePath,bytes)
fs.writeFileSync(path.join(output,'mai-rules.manifest.json'),JSON.stringify({schema:1,browserCommit:'fd270196525c4c6fe61217327c81274e67a58d4b',
  bundleSha256:hash(bytes),bundleSha1:crypto.createHash('sha1').update(bytes).digest('hex'),inputs,adapters:['zustand/vanilla replaces React hook binding','host durable storage replaces IndexedDB I/O; original V0–V3 codec retained','review compute reservation, zero is identity','restart-continuous seeded RNG','semantic native UMG view; no DOM or WebView'],
  sourceModified:false},null,2)+'\n')
console.log(JSON.stringify({bundle:bytes.length,sourceFiles:Object.keys(inputs).length,sha256:hash(bytes),browserRuntime:false}))
