"""Prepare retained Tripo GLBs for gameplay. Requires Pillow; makes no provider calls."""
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import runpy
import argparse
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
NAMES = ['bastion', 'skimmer', 'vector']
WHEELS = ['wheel_front_left', 'wheel_front_right', 'wheel_rear_left', 'wheel_rear_right']

def prepare(name):
    source = ROOT / 'artifacts/model-jobs' / name / 'model.glb'
    raw = source.read_bytes()
    size = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20+size])
    binary = raw[28+size:]
    primitive = doc['meshes'][0]['primitives'][0]
    def read(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3}[a['type']]
        kind = {5126:'f',5125:'I',5123:'H'}[a['componentType']]
        fmt = '<'+kind*n
        start = v.get('byteOffset',0)+a.get('byteOffset',0)
        step = v.get('byteStride',struct.calcsize(fmt))
        return [struct.unpack_from(fmt,binary,start+i*step) for i in range(a['count'])]
    positions = read(primitive['attributes']['POSITION'])
    normals = read(primitive['attributes']['NORMAL'])
    uvs = read(primitive['attributes']['TEXCOORD_0'])
    indices = [i[0] for i in read(primitive['indices'])]
    low = [min(p[i] for p in positions) for i in range(3)]
    high = [max(p[i] for p in positions) for i in range(3)]
    # Provider +X forward/+Y up becomes game +Z forward/+Y up.
    target = {'bastion':(4.6,1.28,2.25),'skimmer':(4.6,1.,2.35),'vector':(4.8,1.,2.35)}[name]
    scales = [target[i]/(high[i]-low[i]) for i in range(3)]
    center = [(low[i]+high[i])/2 for i in range(3)]
    def point(p):
        return (-(p[2]-center[2])*scales[2],(p[1]-low[1])*scales[1]-.65,(p[0]-center[0])*scales[0])
    def normal(n):
        v = [-n[2]/scales[2],n[1]/scales[1],n[0]/scales[0]]
        length = sum(x*x for x in v)**.5
        return tuple(x/length for x in v)
    geo=runpy.run_path(str(ROOT/'scripts/bastion-wheel-geometry.py')) if name=='bastion' else None
    positions=[point(p) for p in positions];normals=[normal(n) for n in normals]
    if geo:
        subtract=runpy.run_path(str(ROOT/'scripts/prepare-clean-cybercar.py'))['subtract']
        body=[]
        for i in range(0,len(indices),3):
            polygons=[[dict(POSITION=positions[v],NORMAL=normals[v],TEXCOORD_0=uvs[v]) for v in indices[i:i+3]]]
            for w in range(4):polygons=[p for polygon in polygons for p in subtract(polygon,geo['well_planes'](w))]
            for polygon in polygons:
                for j in range(1,len(polygon)-1):
                    tri=[polygon[0],polygon[j],polygon[j+1]]
                    cross=geo['shared']['cross'](geo['shared']['sub'](tri[1]['POSITION'],tri[0]['POSITION']),geo['shared']['sub'](tri[2]['POSITION'],tri[0]['POSITION']))
                    if sum(v*v for v in cross)>1e-18:body.extend(tri)
        positions=[v['POSITION'] for v in body];normals=[geo['shared']['unit'](v['NORMAL']) for v in body];uvs=[v['TEXCOORD_0'] for v in body]
        indices=list(range(len(body)))
    out = bytearray(); views=[]; accessors=[]; meshes=[]; nodes=[]
    def view(data):
        while len(out)%4:out.append(0)
        index=len(views);views.append({'buffer':0,'byteOffset':len(out),'byteLength':len(data)});out.extend(data)
        return index
    def accessor(values,kind,n):
        index=len(accessors)
        data=b''.join(struct.pack('<'+kind*n,*v) for v in values)
        a={'bufferView':view(data),'componentType':5126 if kind=='f' else 5125,'count':len(values),'type':{1:'SCALAR',2:'VEC2',3:'VEC3'}[n]}
        if n==3 and kind=='f':a.update(min=[min(v[i] for v in values) for i in range(3)],max=[max(v[i] for v in values) for i in range(3)])
        accessors.append(a);return index
    parts=[]
    def add_mesh(partname,positions,normals,uvs,triangles,material=0,pivot=(0,0,0),radius=0,width=0):
        used=list(dict.fromkeys(triangles)); remap={v:i for i,v in enumerate(used)}
        attrs={'POSITION':accessor([positions[v] for v in used],'f',3),
               'NORMAL':accessor([normals[v] for v in used],'f',3),
               'TEXCOORD_0':accessor([uvs[v] for v in used],'f',2)}
        nodes.append({'name':partname,'mesh':len(meshes)})
        meshes.append({'primitives':[{'attributes':attrs,'indices':accessor([(remap[v],) for v in triangles],'I',1),'material':material}]})
        parts.append({'name':partname,'indices':len(triangles),'pivot':pivot,'radius':radius,'width':width})
    add_mesh('chassis',positions,normals,uvs,indices)
    if geo:
        for w,partname in enumerate(WHEELS):
            mesh=geo['wheel'](w);pivot=geo['CENTERS'][w]
            add_mesh(partname,[tuple(a+b for a,b in zip(p,pivot)) for p in mesh.positions],mesh.normals,mesh.uv,mesh.indices,1,pivot,geo['RADIUS'],geo['HALF_WIDTHS'][w]*2)
        mesh=geo['liner']();add_mesh('wheel_wells',mesh.positions,mesh.normals,mesh.uv,mesh.indices,1)
    images=[]
    for im in doc['images']:
        v=doc['bufferViews'][im['bufferView']]; start=v.get('byteOffset',0)
        with Image.open(io.BytesIO(binary[start:start+v['byteLength']])) as image:
            image=image.resize((1024,1024),Image.Resampling.LANCZOS)
            buffer=io.BytesIO();image.save(buffer,format='PNG')
        images.append({'name':im.get('name','texture'),'mimeType':'image/png','bufferView':view(buffer.getvalue())})
    result={k:copy.deepcopy(doc[k]) for k in ('asset','materials','textures','samplers') if k in doc}
    if geo:
        texture=len(result['textures'])
        for mr in (False,True):
            result['textures'].append(dict(source=len(images)))
            images.append(dict(name='Authored wheel '+('ORM' if mr else 'color'),mimeType='image/png',bufferView=view(geo['shared']['png'](mr))))
        result['materials'].append(dict(name='Authored wheels rubber and alloy',pbrMetallicRoughness=dict(baseColorFactor=[1,1,1,1],
            baseColorTexture=dict(index=texture),metallicFactor=1,roughnessFactor=1,metallicRoughnessTexture=dict(index=texture+1))))
    result.update(buffers=[{'byteLength':len(out)}],bufferViews=views,accessors=accessors,meshes=meshes,nodes=nodes,scenes=[{'nodes':list(range(len(nodes)))}],scene=0,images=images)
    encoded=json.dumps(result,separators=(',',':')).encode()
    encoded+=b' '*((-len(encoded))%4);out+=b'\0'*((-len(out))%4)
    glb=b'glTF'+struct.pack('<II',2,28+len(encoded)+len(out))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+struct.pack('<I4s',len(out),b'BIN\0')+out
    dest=ROOT/'assets/runtime/models'/name;dest.mkdir(parents=True,exist_ok=True)
    (dest/(name+'.glb')).write_bytes(glb)
    job=json.loads(source.with_suffix('.glb.provenance.json').read_text())
    record={'asset':name,'provider':'Tripo3D','task_id':job['task_id'],'source_sha256':hashlib.sha256(raw).hexdigest(),
        'sha256':hashlib.sha256(glb).hexdigest(),'triangles':sum(p['indices'] for p in parts)//3,'texture_size':[1024,1024],
        'dimensions':{'length':target[0],'height':target[1],'width':target[2]},'parts':parts,
        'preparation':'scripts/prepare-vehicles.py; +Y up, +Z forward, metres; clipped Bastion wheel wells and closed authored wheel assemblies' if geo else 'scripts/prepare-vehicles.py; +Y up, +Z forward, metres',
        'license_status':job['license_status']}
    if geo:record.update(suspension=geo['SUSPENSION'],max_steer=geo['MAX_STEER'],wheel_texture_size=[256,32])
    (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
    print(name,len(glb),'bytes',[(p['name'],p['indices']//3) for p in parts])
    return record

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--only',choices=NAMES);args=parser.parse_args()
    records=[prepare(n) if not args.only or args.only==n else json.loads((ROOT/'assets/runtime/models'/n/(n+'.asset.json')).read_text()) for n in NAMES]
    lines=['// Generated by scripts/prepare-vehicles.py. Source GLBs retained in artifacts.',
        '#pragma once','#include "engine/math.hpp"','#include <array>','namespace racing::vehicleart {',
        'struct Part { const char* name; unsigned indices; engine::Vec3 pivot; float radius, width; };',
        'struct Model { const char* name; const char* hash; unsigned count; std::array<Part,6> parts; float suspensionRest, suspensionMin, suspensionMax; };',
        'inline const std::array<Model,3> models{{']
    def f(v):return f'{v:.9f}f'
    for r in records:
        rows=[f'{{"{p["name"]}",{p["indices"]},{{'+','.join(f(v) for v in p['pivot'])+'},'+f(p['radius'])+','+f(p.get('width',0))+'}' for p in r['parts']]
        suspension=r.get('suspension',dict(rest=.30,minimum=.10,maximum=.45))
        lines.append('{"'+r['asset']+'","'+r['sha256']+'",'+str(len(rows))+',{{'+','.join(rows)+'}},'+','.join(f(suspension[k]) for k in ['rest','minimum','maximum'])+'},')
    lines+=['}};','}']
    (ROOT/'src/vehicle_art_contract.hpp').write_text('\n'.join(lines)+'\n')
