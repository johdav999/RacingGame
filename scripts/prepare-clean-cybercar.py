"""Prepare the approved 2026-10-22 cybercar for the existing Vektor wheel rig.

Retains the source body outside authored wheel wells and its embedded 4K images.
Replaces fused source wheels with closed concentric tire/rim/hub assemblies.
No provider calls. Run before scripts/cybercar-contract.py.
"""
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import runpy

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'artifacts/model-jobs/cybercar-clean-detail/model.glb'
SEGMENTED=ROOT/'artifacts/model-jobs/cybercar-clean-detail-parts/model.glb'
DEST=ROOT/'assets/runtime/models/cybercar-clean-detail'
WHEELS=['wheel_front_left','wheel_front_right','wheel_rear_left','wheel_rear_right']

def load(path):
    raw=path.read_bytes();size=struct.unpack_from('<I',raw,12)[0]
    assert raw[:4]==b'glTF' and struct.unpack_from('<I',raw,8)[0]==len(raw)
    return raw,json.loads(raw[20:20+size]),raw[28+size:]

def read(doc,binary,index):
    a=doc['accessors'][index];v=doc['bufferViews'][a['bufferView']]
    n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
    fmt='<'+{5126:'f',5125:'I',5123:'H'}[a['componentType']]*n
    start=v.get('byteOffset',0)+a.get('byteOffset',0);step=v.get('byteStride',struct.calcsize(fmt))
    return [struct.unpack_from(fmt,binary,start+i*step) for i in range(a['count'])]

def subtract(poly, planes):
    """Subtract a convex wheel well, interpolating source attributes at cuts."""
    def distance(v,plane):
        n,d=plane;return sum(a*b for a,b in zip(v['POSITION'],n))-d
    if any(all(distance(v,plane)>1e-10 for v in poly) for plane in planes):return [poly]
    outside=[]
    for plane in planes:
        inside=[];piece=[]
        for a,b in zip(poly,poly[1:]+poly[:1]):
            da,db=distance(a,plane),distance(b,plane)
            (inside if da<=0 else piece).append(a)
            if (da<=0)!=(db<=0):
                t=da/(da-db)
                v={key:tuple(x+(y-x)*t for x,y in zip(a[key],b[key])) for key in a}
                inside.append(v);piece.append(v)
        if len(piece)>=3:outside.append(piece)
        poly=inside
        if len(poly)<3:break
    return outside

def main():
    geo=runpy.run_path(str(ROOT/'scripts/cybercar-wheel-geometry.py'))
    raw,doc,binary=load(SOURCE);segraw,seg,_=load(SEGMENTED)
    primitive=doc['meshes'][0]['primitives'][0]
    streams={k:read(doc,binary,a) for k,a in primitive['attributes'].items()}
    indices=[v[0] for v in read(doc,binary,primitive['indices'])]
    low=min(v[1] for v in streams['POSITION'])
    body=[]
    for start in range(0,len(indices),3):
        polygons=[[{k:s[i] for k,s in streams.items()} for i in indices[start:start+3]]]
        for w in range(4):polygons=[piece for polygon in polygons for piece in subtract(polygon,geo['well_planes'](w))]
        for polygon in polygons:
            for j in range(1,len(polygon)-1):
                tri=[polygon[0],polygon[j],polygon[j+1]]
                normal=geo['cross'](geo['sub'](tri[1]['POSITION'],tri[0]['POSITION']),geo['sub'](tri[2]['POSITION'],tri[0]['POSITION']))
                if sum(v*v for v in normal)>1e-18:body.extend(tri)
    def point(p):return (-p[0],p[1]-low,-p[2])
    def direction(p):return (-p[0],p[1],-p[2])
    out=bytearray();views=[];accessors=[];meshes=[];nodes=[];parts=[]
    def view(data):
        out.extend(b'\0'*(-len(out)%4));index=len(views)
        views.append(dict(buffer=0,byteOffset=len(out),byteLength=len(data)));out.extend(data);return index
    def accessor(values,kind,n):
        index=len(accessors);a=dict(bufferView=view(b''.join(struct.pack('<'+kind*n,*v) for v in values)),
            componentType=5126 if kind=='f' else 5125,count=len(values),type={1:'SCALAR',2:'VEC2',3:'VEC3',4:'VEC4'}[n])
        if kind=='f' and n==3:a.update(min=[min(v[j] for v in values) for j in range(3)],max=[max(v[j] for v in values) for j in range(3)])
        accessors.append(a);return index
    def add_mesh(name,attributes,triangles,material,pivot=None,radius=0):
        attrs={key:accessor(values,'f',len(values[0])) for key,values in attributes.items()}
        node=dict(name=name,mesh=len(meshes))
        if pivot is not None:node.update(translation=pivot,extras=dict(radius_source_units=radius))
        nodes.append(node);meshes.append(dict(name=name,primitives=[dict(attributes=attrs,indices=accessor([(i,) for i in triangles],'I',1),material=material)]))
        parts.append(dict(name=name,triangles=len(triangles)//3,pivot=pivot or [0,0,0],radius=radius))
    attrs={key:[] for key in streams}
    for vertex in body:
        for key,value in vertex.items():
            if key=='POSITION':value=point(value)
            elif key=='NORMAL':value=direction(geo['unit'](value))
            elif key=='TANGENT':value=(*direction(geo['unit'](value[:3])),value[3])
            attrs[key].append(value)
    add_mesh('cybercar_chassis',attrs,list(range(len(body))),0)
    for w,name in enumerate(WHEELS):
        mesh=geo['wheel'](geo['RADII'][w]);pivot=point(geo['CENTERS'][w])
        add_mesh(name,dict(POSITION=[direction(p) for p in mesh.positions],NORMAL=[direction(n) for n in mesh.normals],TEXCOORD_0=mesh.uv),mesh.indices,1,pivot,geo['RADII'][w])
    mesh=geo['liner']()
    add_mesh('cybercar_wheel_wells',dict(POSITION=[point(p) for p in mesh.positions],NORMAL=[direction(n) for n in mesh.normals],TEXCOORD_0=mesh.uv),mesh.indices,1)
    images=[]
    for im in doc['images']:
        v=doc['bufferViews'][im['bufferView']];start=v.get('byteOffset',0)
        image={k:copy.deepcopy(value) for k,value in im.items() if k!='bufferView'}
        image['bufferView']=view(binary[start:start+v['byteLength']]);images.append(image)
    result={k:copy.deepcopy(doc[k]) for k in ('asset','materials','textures','samplers') if k in doc}
    first_image=len(images);first_texture=len(result['textures'])
    for mr in (False,True):
        images.append(dict(name='Authored wheel '+('ORM' if mr else 'color'),mimeType='image/png',bufferView=view(geo['png'](mr))))
        result['textures'].append(dict(source=first_image+int(mr)))
    result['materials'][0]['name']='Cybercar body original 4K PBR'
    result['materials'].append(dict(name='Authored wheels rubber and alloy',pbrMetallicRoughness=dict(baseColorFactor=[1,1,1,1],
        baseColorTexture=dict(index=first_texture),metallicFactor=1,roughnessFactor=1,metallicRoughnessTexture=dict(index=first_texture+1))))
    result.update(buffers=[dict(byteLength=len(out))],bufferViews=views,accessors=accessors,meshes=meshes,nodes=nodes,
        scenes=[dict(nodes=list(range(len(nodes))))],scene=0,images=images)
    encoded=json.dumps(result,separators=(',',':')).encode();encoded+=b' '*(-len(encoded)%4);out+=b'\0'*(-len(out)%4)
    glb=b'glTF'+struct.pack('<II',2,28+len(encoded)+len(out))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+struct.pack('<I4s',len(out),b'BIN\0')+out
    DEST.mkdir(parents=True,exist_ok=True);(DEST/'cybercar.glb').write_bytes(glb)
    job=json.loads(SOURCE.with_suffix('.glb.provenance.json').read_text())
    record=dict(asset='cybercar-clean-detail',provider='Tripo3D with authored replacement wheels',task_id=job['task_id'],segmentation_task_id='c074ff73-19dc-48c4-bb7e-99cfeca58d6a',
        source_sha256=hashlib.sha256(raw).hexdigest(),segmented_sha256=hashlib.sha256(segraw).hexdigest(),sha256=hashlib.sha256(glb).hexdigest(),
        triangles=sum(p['triangles'] for p in parts),texture_size=[4096,4096],wheel_texture_size=[256,32],parts=parts,scale=4.3,bodyOffsetY=-.65,
        suspension=dict(rest=.30,minimum=.24,maximum=.38),max_steer=.42,
        preparation='Source wheels removed by clipped wheel-well apertures; retained body UVs and original image bytes; closed authored tires/rims/spokes/hubs and recessed wheel-well liners',
        license_status=job['license_status'])
    (DEST/'cybercar.asset.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(dict(bytes=len(glb),triangles=record['triangles'],parts=parts),indent=2))

if __name__=='__main__':main()

