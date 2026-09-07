from pathlib import Path
import json,html,csv,hashlib,struct
from PIL import Image,ImageDraw,ImageFont
OUT=Path(__file__).resolve().parents[1]
assets=[json.loads(p.read_text()) for p in sorted(OUT.glob('*/*/asset.json'))]
for a in assets:
 folder=OUT/a['group']/a['id'];recipe=folder/'materials.json';data=json.loads(recipe.read_text());nameset={m['name'] for m in data};raw=(folder/(a['id']+'.glb')).read_bytes();size=struct.unpack_from('<I',raw,12)[0];gltf=json.loads(raw[20:20+size])
 for m in gltf.get('materials',[]):
  if m.get('name') not in nameset:
   pbr=m.get('pbrMetallicRoughness',{});data.append({'name':m.get('name'),'base_color_linear':pbr.get('baseColorFactor',[1,1,1,1]),'metallic':pbr.get('metallicFactor',1),'roughness_scalar':pbr.get('roughnessFactor',1),'texture_family':None,'emission_color_linear':m.get('emissiveFactor',[0,0,0]),'emission_strength':0})
 recipe.write_text(json.dumps(data,ensure_ascii=False,indent=2))
old={a['id']:a for a in json.loads((OUT/'source-inventory.json').read_text())}
labels={'vehicles':'Транспорт','characters':'Персонажи','racks':'Серверные стойки','interiors':'Интерьеры'}
names={'car-1':'Бирюзовый седан','car-2':'Грузовой фургон','car-3':'Компактный хетчбэк','car-4':'Спортивное купе','car-5':'Автобус','car-6':'Такси','person-1':'Рубашка и чиносы','person-2':'Синяя рубашка','person-3':'Деловой костюм','person-4':'Топ и джинсы','person-5':'Блузка и юбка','person-6':'Топ с принтом','rack-basic':'EDGE / базовая','rack-cooled':'FLOW / охлаждаемая','rack-enterprise':'CORE / старшая','garage':'Гараж','workshop':'Мастерская','technopark':'Технопарк','server-hall':'Серверный цех','campus':'Кампус','hq':'Штаб-квартира','dc-north':'Северный дата-центр','dc-south':'Южный дата-центр'}
fontpath='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
font=ImageFont.truetype(fontpath,25);small=ImageFont.truetype(fontpath,18);title=ImageFont.truetype(fontpath,38)
previews=OUT/'previews';previews.mkdir(exist_ok=True)
for group,label in labels.items():
 aa=[a for a in assets if a['group']==group];cols=3 if len(aa)<=6 else 4;cell=480;rows=(len(aa)+cols-1)//cols;im=Image.new('RGB',(cols*cell,110+rows*(cell+66)),(22,28,36));draw=ImageDraw.Draw(im);draw.text((25,20),'NEURON / '+label,font=title,fill=(233,239,243));draw.text((27,70),'Новые 3D-модели • контрольные рендеры Blender Cycles',font=small,fill=(157,182,199))
 for i,a in enumerate(aa):
  x=(i%cols)*cell;y=110+(i//cols)*(cell+66);p=OUT/group/a['id']/(a['id']+'.png');pic=Image.open(p).convert('RGB');pic.thumbnail((cell-8,cell-8));im.paste(pic,(x+4,y+4));draw.text((x+14,y+cell+4),a['id']+' / '+names[a['id']],font=small,fill=(232,238,242));draw.text((x+14,y+cell+31),f"{a['triangles']:,} треуг. · {a['objects']} мешей".replace(',',' '),font=small,fill=(134,169,191))
 im.save(previews/(group+'.png'),optimize=True)
# CSV and structured manifest are the exact numerical catalog, independent of presentation.
with (OUT/'asset-catalog.csv').open('w',newline='',encoding='utf-8-sig') as f:
 w=csv.writer(f);w.writerow(['id','category','name','source_triangles','new_triangles','enclosure_triangles','mesh_objects','bones','width_m','depth_m','height_m','fbx_bytes','glb_bytes'])
 for a in assets:w.writerow([a['id'],a['group'],names[a['id']],old[a['id']]['triangles'],a['triangles'],a['enclosure_triangles'],a['objects'],a['bones'],*a['dimensions_m'],a['fbx_bytes'],a['glb_bytes']])
summary={'source_commit':'fd270196525c4c6fe61217327c81274e67a58d4b','asset_count':len(assets),'mesh_objects':sum(a['objects'] for a in assets),'triangles':sum(a['triangles'] for a in assets),'enclosure_triangles':sum(a['enclosure_triangles'] for a in assets),'fbx_bytes':sum(a['fbx_bytes'] for a in assets),'glb_bytes':sum(a['glb_bytes'] for a in assets),'unreal_editor_verified':False,'source_repository_modified':False,'city_v3_modified':False,'assets':assets}
(OUT/'manifest.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2))
rows=[]
for a in assets:
 id=a['id'];group=a['group'];folder=f'{group}/{id}';dims=' × '.join(f'{v:.2f}' for v in a['dimensions_m']);rows.append(f'<article data-group="{group}"><a href="{folder}/{id}.png"><img loading="lazy" src="{folder}/{id}.png" alt="{html.escape(names[id])}"></a><div><h2>{html.escape(names[id])} <small>{id}</small></h2><p>{a["triangles"]:,} треугольников · {dims} м</p><nav><a download href="{folder}/{id}.png">PNG</a><a download href="{folder}/{id}.blend">Blender</a><a download href="{folder}/{id}.fbx">FBX</a><a download href="{folder}/{id}.glb">GLB</a></nav></div></article>')
page='''<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Neuron — 3D core</title><style>*{box-sizing:border-box}body{margin:0;background:#141a22;color:#e6edf3;font:16px system-ui}header,main{max-width:1440px;margin:auto;padding:28px}h1{font-size:clamp(32px,6vw,64px);margin:0}header p{max-width:800px;line-height:1.6;color:#a8bdcd}.filters{display:flex;gap:8px;flex-wrap:wrap}button,nav a{border:1px solid #40566a;background:#22303e;color:#d9eef9;border-radius:8px;padding:9px 15px;cursor:pointer;text-decoration:none}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:18px}article{border:1px solid #334250;border-radius:12px;overflow:hidden;background:#1c2631}article img{width:100%;display:block}article div{padding:16px}h2{font-size:19px}small{font-size:13px;color:#9bb1c4}article p{font-size:14px;color:#9db3c5}nav{display:flex;gap:7px;flex-wrap:wrap}nav a{padding:7px 10px;font-size:13px}a{color:#81c9ea}.hidden{display:none}</style><header><h1>NEURON / 3D CORE</h1><p>23 самостоятельных ассета из репозитория: обновлённая геометрия, материалы и обменные форматы. Город v3 сохранён отдельно. Это рендеры Blender, не скриншоты Unreal. Физика и проверка в UE5 — следующий этап.</p><p><a href="UNREAL_IMPORT_RU.md">Инструкция импорта</a> · <a href="asset-catalog.csv">Каталог CSV</a> · <a href="validation.json">Результаты проверки</a></p><div class="filters"><button data-g="all">Все</button><button data-g="vehicles">Транспорт</button><button data-g="characters">Персонажи</button><button data-g="racks">Стойки</button><button data-g="interiors">Интерьеры</button></div></header><main>'''+''.join(rows)+'''</main><script>document.querySelectorAll('button').forEach(b=>b.onclick=()=>document.querySelectorAll('article').forEach(a=>a.classList.toggle('hidden',b.dataset.g!=='all'&&a.dataset.group!==b.dataset.g)))</script></html>'''
(OUT/'index.html').write_text(page)
md=['# Neuron — обновлённое 3D-ядро','',f"{len(assets)} моделей; {summary['mesh_objects']} мешей; {summary['triangles']:,} треугольников в основных экспортах. Дополнительные оболочки помещений: {summary['enclosure_triangles']:,} треугольников.",'','| Модель | Назначение | Было треуг. | Стало треуг. | FBX, МБ |','|---|---|---:|---:|---:|']
for a in assets:md.append(f"| {a['id']} | {names[a['id']]} | {old[a['id']]['triangles']:,} | {a['triangles']:,} | {a['fbx_bytes']/1e6:.2f} |")
md+=['','Исходники .blend, обменные .fbx/.glb, PNG каждой модели, PBR-карты, параметры материалов и генераторы включены. Открыть index.html после распаковки для просмотра каталога.','','Модели остаются стилизованными. Увеличение деталей не выдаётся за реалистичный AAA-скульпт. UE5 в среде сборки отсутствует: импорт, свет, Nanite, физика, игровые анимации и производительность в движке не подтверждены. Подробности — UNREAL_IMPORT_RU.md.']
(OUT/'README_RU.md').write_text('\n'.join(md),encoding='utf-8')
print('CATALOG',len(assets),summary['triangles'])
