"""Offline GLB packaging checks and contact sheets for the generated desert kit."""
from pathlib import Path
import hashlib,io,json,struct
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
specs=json.loads((ROOT/'docs/desert-solar-prompts.json').read_text())['assets']
records=[]
for spec in specs:
 name=spec['id'];folder=ROOT/'assets/runtime/models/desert-solar'/name
 record=json.loads((folder/(name+'.asset.json')).read_text())
 blob=(folder/(name+'.glb')).read_bytes();length=struct.unpack_from('<I',blob,12)[0]
 doc=json.loads(blob[20:20+length]);binary=blob[28+length:]
 assert blob[:4]==b'glTF' and struct.unpack_from('<I',blob,8)[0]==len(blob)
 assert record['sha256']==hashlib.sha256(blob).hexdigest()
 count=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
 assert count==record['triangles']<=spec['budget']<=20000
 maps=[]
 for im in doc['images']:
  assert 'uri' not in im,'External texture dependency'
  view=doc['bufferViews'][im['bufferView']];offset=view.get('byteOffset',0)
  image=Image.open(io.BytesIO(binary[offset:offset+view['byteLength']]))
  assert max(image.size)<=record['texture_dimension_cap'],(name,image.size)
  maps.append({'size':image.size,'format':image.format})
 for mat in doc['materials']:
  assert 'baseColorTexture' in mat['pbrMetallicRoughness']
  if mat.get('name')!='solar-mirror-glass':
   assert 'metallicRoughnessTexture' in mat['pbrMetallicRoughness']
   assert 'normalTexture' in mat
  if 'conduit' in name or 'receiver' in name:assert 'emissiveTexture' in mat
 record['exported_maps']=maps;records.append(record)
out=ROOT/'assets/runtime/models/desert-solar'
report={'asset_count':len(records),'total_triangles':sum(r['triangles'] for r in records),
 'total_bytes':sum(r['bytes'] for r in records),'credits_consumed':sum(r.get('credits_consumed') or 0 for r in records),
 'checks':'GLB structure, hashes, embedded PBR/normal/emissive maps, map dimensions and actual triangles',
 'assets':records}
(out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',17)
for side in ['front','back']:
 sheet=Image.new('RGB',(1440,1200),(24,29,35));draw=ImageDraw.Draw(sheet)
 for i,spec in enumerate(specs):
  name=spec['id'];tile=Image.open(ROOT/'artifacts/model-jobs/desert-solar'/name/'qa'/(side+'.png')).convert('RGB')
  tile.thumbnail((352,352));x=(i%4)*360+4;y=(i//4)*400
  sheet.paste(tile,(x,y));draw.text((x+4,y+357),name,font=font,fill='white')
  draw.text((x+4,y+379),str(records[i]['triangles'])+' triangles',font=font,fill=(128,218,227))
 sheet.save(ROOT/'docs/images'/('desert-solar-models-'+side+'.jpg'),quality=92)
print(json.dumps({k:v for k,v in report.items() if k!='assets'},indent=2))
