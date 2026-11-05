"""Reimport track GLBs, verify surface heights and joins, and render review views."""
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
spec=importlib.util.spec_from_file_location('track_tools',ROOT/'scripts/prepare-dockside-track-surfaces.py')
P=importlib.util.module_from_spec(spec)
spec.loader.exec_module(P)
H=P.H
QA=P.QA/'verified'
DOC=ROOT/'docs/images/dockside-track-surfaces'
IDS=['track-straight','track-corner-left','track-corner-right','track-curb','drainage-grate']

def load(name,reset=True):
    if reset:bpy.ops.wm.read_factory_settings(use_empty=True)
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(P.DEST/(name+'.glb')))
    return [o for o in bpy.context.scene.objects if o not in before and o.type=='MESH']

def tree(objects):
    vertices=[]
    faces=[]
    for o in objects:
        offset=len(vertices)
        vertices.extend(o.matrix_world@v.co for v in o.data.vertices)
        faces.extend(tuple(offset+i for i in p.vertices) for p in o.data.polygons)
    return BVHTree.FromPolygons(vertices,faces)

def hit_height(bvh,x,y):
    hit=bvh.ray_cast(Vector((x,y,2)),Vector((0,0,-1)),3)[0]
    return hit.z if hit is not None else None

def review():
    records=[]
    for name in IDS:
        path=P.DEST/(name+'.glb')
        raw=path.read_bytes()
        assert struct.unpack_from('<4sII',raw)==(b'glTF',2,len(raw))
        doc=json.loads(raw[20:20+struct.unpack_from('<I',raw,12)[0]])
        assert all('uri' not in b for b in doc['buffers'])
        assert all('bufferView' in im and 'uri' not in im for im in doc['images'])
        for material in doc['materials']:
            if material['name']=='Dark concrete road substrate':continue
            assert 'baseColorTexture' in material['pbrMetallicRoughness']
            assert 'metallicRoughnessTexture' in material['pbrMetallicRoughness']
            assert 'normalTexture' in material
        meta=json.loads(path.with_suffix('.asset.json').read_text())
        assert hashlib.sha256(raw).hexdigest()==meta['sha256']
        objects=load(name)
        count=sum(H.triangles(o) for o in objects)
        assert 0<count<=20000 and count==meta['triangles_instanced']
        lo,hi=H.bounds(objects,True)
        size=hi-lo
        measured=[size.x,size.z,size.y]
        assert all(abs(a-b)<.001 for a,b in zip(measured,meta['dimensions_xyz_gltf']))
        assert abs(lo.z)<.001
        assert all(math.isfinite(c) for o in objects for v in o.data.vertices for c in v.co)
        maps=[{'name':im.name,'size':list(im.size)} for im in bpy.data.images if im.type=='IMAGE']
        assert all(max(im['size'])<=meta['texture_cap'] for im in maps)
        bvh=tree(objects)
        heights=[]
        if name=='track-straight':
            heights=[hit_height(bvh,x,y) for x in [-5.9,-3,0,3,5.9] for y in [-7.9,-4,0,4,7.9]]
        elif 'corner' in name:
            mirror=-1 if name.endswith('right') else 1
            for angle in range(1,90,4):
                theta=math.radians(angle)
                for radius in [4.1,6,10,14,15.9]:
                    heights.append(hit_height(bvh,mirror*(radius*math.cos(theta)-8),radius*math.sin(theta)-8))
                assert hit_height(bvh,mirror*(3*math.cos(theta)-8),3*math.sin(theta)-8) is None
                assert hit_height(bvh,mirror*(16.2*math.cos(theta)-8),16.2*math.sin(theta)-8) is None
        if heights:assert all(h is not None and abs(h-.25)<.0001 for h in heights),(name,heights)
        record={'id':name,'sha256_verified':True,'embedded_pbr_verified':True,'triangles':count,
            'dimensions_xyz_gltf':measured,'maps':maps,'flat_top_samples':len(heights)}
        if name=='drainage-grate':
            hits=[hit_height(bvh,-.9+(i+.31)*1.8/80,-.18+(j+.47)*.36/12) for i in range(80) for j in range(12)]
            record['open_slot_sample_fraction']=sum(h is None for h in hits)/len(hits)
            assert record['open_slot_sample_fraction']>.15
        H.render(objects,QA/(name+'-front.png'),(-1,-1,1.8),size=(1200,1000))
        H.render(objects,QA/(name+'-back.png'),(1,1,1.8),size=(1200,1000))
        H.render(objects,QA/(name+'-top.png'),(0,-.001,3),size=(1000,1000))
        records.append(record)
        print('VERIFIED '+json.dumps(record),flush=True)
    H.save_json(P.DEST/'verification.json',{'scope':'Exported GLB reimport, hashes, PBR, meter bounds, pivots, triangle counts, surface ray tests and renders; no game physics acceptance',
        'assets':records})

def junction():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    objects=[]
    placements=[]
    specs=[('track-corner-left',(0,0,0),0),('track-straight',(2,-16,0),0),('track-straight',(-16,2,0),90)]
    for name,location,angle in specs:
        imported=load(name,reset=False)
        for obj in imported:
            obj.matrix_world=Matrix.Translation(location)@Matrix.Rotation(math.radians(angle),4,'Z')@obj.matrix_world
        objects.extend(imported)
        placements.append({'asset':name,'translation_xyz_gltf':[location[0],location[2],-location[1]],'rotation_y_degrees':angle})
    bpy.context.view_layer.update()
    # Sample both sides of both seams across the full usable road width.
    bvh=tree(objects)
    seam_samples=0
    for u in [-5.9,-3,0,3,5.9]:
        for delta in [-.001,.001]:
            for x,y in [(2+u,-8+delta),(-8+delta,2+u)]:
                height=hit_height(bvh,x,y)
                assert height is not None and abs(height-.25)<.0001,(x,y,height)
                seam_samples+=1
    H.render(objects,DOC/'road-join.png',(-1,-1,2),size=(1500,1200))
    bpy.ops.wm.save_as_mainfile(filepath=str(P.QA/'road-join.blend'),compress=True)
    H.save_json(P.DEST/'example-layout.json',{'units':'meters','up_axis':'Y','components':placements,
        'join_samples_passed':seam_samples,'road_width_m':12,'surface_y_m':.25})

def gallery():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    objects=[]
    for index,name in enumerate(IDS):
        imported=load(name,reset=False)
        lo,hi=H.bounds(imported,True)
        center=(lo+hi)/2
        rotation=Matrix.Rotation(math.radians(65),4,'X')@Matrix.Rotation(math.radians(-20),4,'Z')
        coords=[rotation@(o.matrix_world@v.co-center) for o in imported for v in o.data.vertices]
        factor=4.5/max(max(v.x for v in coords)-min(v.x for v in coords),max(v.z for v in coords)-min(v.z for v in coords))
        for obj in imported:
            for vertex in obj.data.vertices:vertex.co=rotation@((obj.matrix_world@vertex.co-center)*factor)
            obj.matrix_world=Matrix.Identity(4)
            obj.location=((index%3-1)*6.5,0,(.5-index//3)*5.5)
        objects.extend(imported)
        font=bpy.data.curves.new(name+' label','FONT')
        font.body=name.replace('track-','').replace('-',' ').title()
        font.align_x='CENTER'
        font.size=.28
        ob=bpy.data.objects.new(name+' label',font)
        bpy.context.scene.collection.objects.link(ob)
        ob.location=((index%3-1)*6.5,-3,(.5-index//3)*5.5-2.55)
        ob.rotation_euler=(math.pi/2,0,0)
        mat=bpy.data.materials.get('Labels') or bpy.data.materials.new('Labels')
        mat.use_nodes=True
        shader=mat.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Base Color'].default_value=(.85,.88,.92,1)
        shader.inputs['Emission'].default_value=(.85,.88,.92,1)
        shader.inputs['Emission Strength'].default_value=.5
        font.materials.append(mat)
    H.render(objects,DOC/'kit-preview.png',(0,-3,0),size=(1800,1100))

if __name__=='__main__':
    QA.mkdir(parents=True,exist_ok=True)
    DOC.mkdir(parents=True,exist_ok=True)
    review()
    junction()
    gallery()
