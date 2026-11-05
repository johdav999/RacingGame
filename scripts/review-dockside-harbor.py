"""Reimport the delivered harbor GLBs, verify budgets/maps/scale, and render QA."""
import hashlib
import importlib.util
import json
import math
import struct
from pathlib import Path

import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('harbor_tools',ROOT/'scripts/prepare-dockside-harbor.py')
P=importlib.util.module_from_spec(spec)
spec.loader.exec_module(P)
H=P.H
QA=P.QA/'verified'
DOC=ROOT/'docs/images/dockside-harbor'

def load(name,reset=True):
    if reset:bpy.ops.wm.read_factory_settings(use_empty=True)
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(P.DEST/(name+'.glb')))
    return [o for o in bpy.context.scene.objects if o not in before and o.type=='MESH']

def review():
    records=[]
    for name in P.ASSETS:
        path=P.DEST/(name+'.glb')
        raw=path.read_bytes()
        magic,version,length=struct.unpack_from('<4sII',raw)
        assert magic==b'glTF' and version==2 and length==len(raw)
        doc=json.loads(raw[20:20+struct.unpack_from('<I',raw,12)[0]])
        assert all('uri' not in b for b in doc['buffers'])
        assert all('bufferView' in im and 'uri' not in im for im in doc['images'])
        meta=json.loads(path.with_suffix('.asset.json').read_text())
        assert hashlib.sha256(raw).hexdigest()==meta['sha256']
        for material in doc['materials']:
            if material['name']=='Galvanized_chainlink_wire':continue
            assert 'baseColorTexture' in material['pbrMetallicRoughness']
            assert 'metallicRoughnessTexture' in material['pbrMetallicRoughness']
            assert 'normalTexture' in material
        objects=load(name)
        count=sum(H.triangles(o) for o in objects)
        assert 0<count<=20000 and count==meta['triangles_instanced']
        lo,hi=H.bounds(objects,True)
        size=hi-lo
        expected=meta['dimensions_xyz_gltf']
        measured=[size.x,size.z,size.y]
        assert all(abs(a-b)<.001 for a,b in zip(measured,expected)),(name,measured,expected)
        assert abs(lo.z)<.001
        assert all(math.isfinite(c) for o in objects for v in o.data.vertices for c in v.co)
        maps=[{'name':im.name,'size':list(im.size)} for im in bpy.data.images if im.type=='IMAGE']
        assert all(max(im['size'])<=meta['texture_cap'] for im in maps)
        H.render(objects,QA/(name+'-front.png'),(-1.4,-2,1.15),size=(1200,1000))
        H.render(objects,QA/(name+'-back.png'),(1.4,2,1.15),size=(1200,1000))
        if name in {'wall-lamp','floodlight-mast'}:
            for side,direction in [('left',(-2,0,.02)),('right',(2,0,.02)),('low-front',(0,-2,.02)),('low-back',(0,2,.02))]:
                H.render(objects,QA/(name+'-'+side+'.png'),direction,size=(1200,1000))
        record={'id':name,'sha256_verified':True,'embedded_maps_verified':True,
            'triangles':count,'dimensions_xyz_gltf':measured,'maps':maps}
        if name=='chainlink-fence':
            vertices=[o.matrix_world@v.co for o in objects for v in o.data.vertices]
            faces=[]
            offset=0
            for o in objects:
                faces.extend(tuple(offset+i for i in p.vertices) for p in o.data.polygons)
                offset+=len(o.data.vertices)
            tree=BVHTree.FromPolygons(vertices,faces)
            hits=0
            samples=0
            for i in range(40):
                for j in range(24):
                    x=-1.75+(i+.37)*3.5/40
                    z=.3+(j+.61)*1.8/24
                    hit=tree.ray_cast(Vector((x,-2,z)),Vector((0,1,0)),4)[0]
                    hits+=hit is not None
                    samples+=1
            record['open_area_sample_fraction']=1-hits/samples
            assert record['open_area_sample_fraction']>.60
        records.append(record)
        print('VERIFIED '+name,flush=True)
    H.save_json(P.DEST/'verification.json',{'scope':'GLB reimport, embedded PBR, finite geometry, meter bounds, pivots, triangle budgets, opposite-view renders; no engine integration',
        'assets':records})

def gallery():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    all_objects=[]
    names=[n for n in P.ASSETS if n!='container-crane']
    for index,name in enumerate(names):
        objects=load(name,reset=False)
        lo,hi=H.bounds(objects,True)
        center=(lo+hi)/2
        rotation=Matrix.Rotation(math.radians(15 if name not in {'wall-lamp','floodlight-mast'} else -10),4,'X')@Matrix.Rotation(math.radians(-25),4,'Z')
        rotated=[rotation@(o.matrix_world@v.co-center) for o in objects for v in o.data.vertices]
        screen_width=max(v.x for v in rotated)-min(v.x for v in rotated)
        screen_height=max(v.z for v in rotated)-min(v.z for v in rotated)
        factor=3.8/max(screen_width,screen_height)
        for obj in objects:
            for vertex in obj.data.vertices:vertex.co=rotation@((obj.matrix_world@vertex.co-center)*factor)
            obj.matrix_world=Matrix.Identity(4)
            obj.location=((index%3-1)*5.8,0,(1-index//3)*5)
        all_objects.extend(objects)
        font=bpy.data.curves.new(name+' label','FONT')
        font.body=name.replace('-',' ').title()
        font.align_x='CENTER'
        font.size=.28
        label=bpy.data.objects.new(name+' label',font)
        bpy.context.scene.collection.objects.link(label)
        label.location=((index%3-1)*5.8,-.2,(1-index//3)*5-2.4)
        label.rotation_euler=(math.pi/2,0,0)
        mat=bpy.data.materials.get('Gallery label') or bpy.data.materials.new('Gallery label')
        mat.use_nodes=True
        shader=mat.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Base Color'].default_value=(.85,.88,.92,1)
        shader.inputs['Emission'].default_value=(.85,.88,.92,1)
        shader.inputs['Emission Strength'].default_value=.5
        font.materials.append(mat)
    H.render(all_objects,DOC/'kit-preview.png',(0,-3,0),size=(1800,1500))

if __name__=='__main__':
    QA.mkdir(parents=True,exist_ok=True)
    DOC.mkdir(parents=True,exist_ok=True)
    review()
    gallery()
