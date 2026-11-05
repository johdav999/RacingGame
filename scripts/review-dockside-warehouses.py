"""Reimport delivered modules/warehouses and check geometry, layout and closure."""
import hashlib
import importlib.util
import json
import math
import struct
from collections import Counter
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('warehouse',ROOT/'scripts/prepare-dockside-warehouses.py')
kit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(kit)
DEST,QA=kit.DEST,kit.QA


def load(path):
    raw=path.read_bytes()
    assert raw[:4]==b'glTF' and struct.unpack_from('<I',raw,4)[0]==2
    assert struct.unpack_from('<I',raw,8)[0]==len(raw)
    size=struct.unpack_from('<I',raw,12)[0]
    doc=json.loads(raw[20:20+size])
    assert all('uri' not in b for b in doc['buffers'])
    assert all('bufferView' in i for i in doc['images'])
    counts=[sum(doc['accessors'][p['indices']]['count']//3 for p in m['primitives']) for m in doc['meshes']]
    instanced=sum(counts[n['mesh']] for n in doc['nodes'] if 'mesh' in n)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    bpy.context.view_layer.update()
    assert sum(kit.triangles(o) for o in objects)==instanced
    assert all(math.isfinite(v) for o in objects for vertex in o.data.vertices for v in vertex.co)
    sizes=[list(im.size) for im in bpy.data.images if im.type=='IMAGE']
    assert sizes and all(w==1024 and h==1024 for w,h in sizes)
    return objects,doc,instanced,sizes


def mesh_tree(objects):
    vertices,faces=[],[]
    for obj in objects:
        offset=len(vertices)
        vertices.extend(obj.matrix_world@v.co for v in obj.data.vertices)
        obj.data.calc_loop_triangles()
        faces.extend(tuple(offset+i for i in tri.vertices) for tri in obj.data.loop_triangles)
    return BVHTree.FromPolygons(vertices,faces,all_triangles=True)


module_reports=[]
for name,settings in kit.MODULES.items():
    record=json.loads((DEST/'modules'/(name+'.asset.json')).read_text())
    for variant in ['detail','assembly']:
        filename=name if variant=='detail' else name+'-assembly'
        path=DEST/'modules'/(filename+'.glb')
        objects,doc,count,sizes=load(path)
        expected=record if variant=='detail' else record['assembly_lod']
        assert hashlib.sha256(path.read_bytes()).hexdigest()==expected['sha256']
        assert count==expected['triangles_instanced']<=settings[variant+'_budget']
        lo,hi=kit.bounds(objects,True)
        assert max(abs((hi-lo)[i]-settings['size'][i]) for i in range(3))<.00002
        assert abs(lo.z)<.00002
        for mat in doc['materials']:
            assert 'normalTexture' in mat and 'metallicRoughnessTexture' in mat['pbrMetallicRoughness']
        module_reports.append({'module':name,'variant':variant,'triangles':count,'dimensions_blender':list(hi-lo),
                               'embedded_maps':len(sizes),'sha256':expected['sha256']})
        if variant=='assembly':kit.render(objects,QA/'verified'/(name+'-assembly.png'),(-1,-2,1))


building_reports=[]
for building in kit.BUILDINGS:
    name=building['id']
    path=DEST/'warehouses'/(name+'.glb')
    record=json.loads((DEST/'warehouses'/(name+'.assembly.json')).read_text())
    objects,doc,count,sizes=load(path)
    assert count==record['triangles_instanced']<=20000
    assert hashlib.sha256(path.read_bytes()).hexdigest()==record['sha256']
    assert len(objects)==len(record['components'])
    assert Counter(o['module'] for o in objects)==Counter(record['module_counts'])
    expected={c['name']:c for c in record['components']}
    for obj in objects:
        c=expected[obj.name]
        p=obj.matrix_world.translation
        assert max(abs(v-w) for v,w in zip((p.x,p.z,-p.y),c['translation']))<.00003
    lo,hi=kit.bounds(objects,True)
    assert abs(lo.z)<.00002
    measured=[hi.x-lo.x,hi.z-lo.z,hi.y-lo.y]
    assert max(abs(v-w) for v,w in zip(measured,record['dimensions_m_xyz_gltf']))<.00005
    tree=mesh_tree(objects)
    offset=Vector((building['nx']*2,building['ny']*2,0))
    rays=0
    for x,y in building['cells']:
        for u in [.1,.5,.9]:
            for v in [.1,.5,.9]:
                point=Vector((4*(x+u),4*(y+v),hi.z+2))-offset
                hit=tree.ray_cast(point,Vector((0,0,-1)),hi.z+3)[0]
                assert hit is not None and hit.z>=kit.FOUNDATION+building['height']-.06,(name,'roof gap',point)
                rays+=1
    for edge in kit.boundary(building['cells']):
        theta=math.radians(edge['angle'])
        outward=Vector((math.sin(theta),-math.cos(theta),0))
        along=Vector((math.cos(theta),math.sin(theta),0))
        for u in [-1.6,0,1.6]:
            for height in [.3,building['height']/2,building['height']-.3]:
                center=Vector((*edge['xy'],kit.FOUNDATION+height))-offset+along*u
                hit=tree.ray_cast(center+outward*1.0,-outward,1.8)[0]
                assert hit is not None,(name,'wall gap',center)
                rays+=1
    empty=0
    cells=set(map(tuple,building['cells']))
    for x in range(building['nx']):
        for y in range(building['ny']):
            if (x,y) in cells:continue
            point=Vector((4*x+2,4*y+2,hi.z+2))-offset
            assert tree.ray_cast(point,Vector((0,0,-1)),hi.z+3)[0] is None,(name,'filled courtyard')
            empty+=1
    kit.render(objects,QA/'verified'/(name+'-front.png'))
    kit.render(objects,QA/'verified'/(name+'-back.png'),(1.5,2,1.3))
    building_reports.append({'id':name,'triangles_instanced':count,'mesh_instances':len(objects),
        'unique_meshes':len(doc['meshes']),'dimensions_m_xyz_gltf':measured,
        'embedded_maps':len(sizes),'closure_rays_passed':rays,'empty_cells_passed':empty,
        'module_placements_verified':True,'sha256':record['sha256']})

kit.save_json(DEST/'verification.json',{'modules':module_reports,'warehouses':building_reports,
    'scope':'Blender 3.5 GLB reimport, payload validation, layout and sampled mesh closure; no game runtime acceptance'})
print('VERIFIED '+json.dumps(building_reports),flush=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
objects=[]
for building,offset in zip(kit.BUILDINGS,[(0,0),(36,0),(0,32),(36,32)]):
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(DEST/'warehouses'/(building['id']+'.glb')))
    new=[o for o in bpy.context.scene.objects if o not in before and o.type=='MESH']
    for obj in new:
        obj.location.x+=offset[0]
        obj.location.y+=offset[1]
    objects.extend(new)
bpy.context.view_layer.update()
kit.render(objects,QA/'warehouse-lineup.png',(1.35,-2,2.25),(1800,1400))
