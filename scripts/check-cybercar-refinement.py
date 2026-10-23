"""Check preserved wheel streams and authored rear surfaces against the retained source."""
import hashlib
import json
import math
from pathlib import Path
import struct
import io
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT/'assets/runtime/models/cybercar1'


def load(name):
    raw=(ASSETS/name).read_bytes()
    size=struct.unpack_from('<I',raw,12)[0]
    return json.loads(raw[20:20+size]),raw[28+size:],raw


def stream(doc,binary,index):
    a=doc['accessors'][index];v=doc['bufferViews'][a['bufferView']]
    n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
    fmt='<'+{5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]*n
    start=v.get('byteOffset',0)+a.get('byteOffset',0)
    stride=v.get('byteStride',struct.calcsize(fmt))
    return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]


original,ob,source=load('cybercar1_parts_textured.glb')
revised,rb,result=load('cybercar1_rear_refined.glb')
record=json.loads((ASSETS/'cybercar1_rear_refined.asset.json').read_text())
assert hashlib.sha256(source).hexdigest()==record['source_sha256']=='230d8278a968f9b2389018201efced83ddd07f7e447bcc05f23ad1db2b3d5331'
assert hashlib.sha256(result).hexdigest()==record['sha256']
count=0
for old in original['nodes']:
    if not old.get('name','').startswith('wheel_'): continue
    new=next(n for n in revised['nodes'] if n['name']==old['name'])
    assert new['translation']==old['translation'] and new['extras']==old['extras']
    a=original['meshes'][old['mesh']]['primitives'][0]
    b=revised['meshes'][new['mesh']]['primitives'][0]
    assert set(a['attributes'])==set(b['attributes'])
    for attr in a['attributes']:
        assert stream(original,ob,a['attributes'][attr])==stream(revised,rb,b['attributes'][attr]),(old['name'],attr)
    assert stream(original,ob,a['indices'])==stream(revised,rb,b['indices'])
    count+=1
assert count==4
authored=0
for node in revised['nodes']:
    if not node['name'].startswith('rear_'): continue
    p=revised['meshes'][node['mesh']]['primitives'][0]
    positions=stream(revised,rb,p['attributes']['POSITION'])
    normals=stream(revised,rb,p['attributes']['NORMAL'])
    uvs=stream(revised,rb,p['attributes']['TEXCOORD_0'])
    assert len(set(uvs))>10, 'Rear must have a real UV atlas'
    assert all(math.isfinite(v) and -.00001<=v<=1.00001 for uv in uvs for v in uv)
    material=revised['materials'][p['material']]['pbrMetallicRoughness']
    for semantic in ['baseColorTexture','metallicRoughnessTexture']:
        texture=revised['textures'][material[semantic]['index']]
        image=revised['images'][texture['source']];view=revised['bufferViews'][image['bufferView']]
        pixels=Image.open(io.BytesIO(rb[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']]))
        assert pixels.size==(1024,1024)
        assert any(lo!=hi for lo,hi in pixels.convert('RGB').getextrema()), 'Atlas must contain baked surface variation'
    indices=[v[0] for v in stream(revised,rb,p['indices'])]
    assert all(all(math.isfinite(v) for v in p) for p in positions)
    assert all(abs(sum(v*v for v in n)-1)<1e-5 for n in normals)
    for i in range(0,len(indices),3):
        a,b,c=[positions[j] for j in indices[i:i+3]]
        u=[b[j]-a[j] for j in range(3)];v=[c[j]-a[j] for j in range(3)]
        cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        assert sum(x*x for x in cross)>1e-18,(node['name'],i)
        assert sum(cross[j]*normals[indices[i]][j] for j in range(3))>0,(node['name'],i)
    authored+=len(indices)//3
assert authored>0
assert record['parts']==15 and record['triangles']<20000
print(f'PASS: four exact wheel streams/pivots/radii; source/output hashes; {authored} rear triangles with valid normals/UVs and two 1024px baked maps.')
