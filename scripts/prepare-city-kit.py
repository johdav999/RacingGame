"""Normalize retained Tripo city-kit GLBs and generate their runtime contract.

Run in Blender: --background --python scripts/prepare-city-kit.py -- [asset ...]
No provider calls. Source geometry and UVs remain intact; maps are resized.
"""
import bpy
import hashlib
import json
import struct
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
# Height in metres, except catwalk which uses its longest horizontal span.
SPECS = {'market-stall': 3.0, 'closed-kiosk': 3.2, 'dumpster': 1.5,
         'crate-stack': 1.5, 'bollard': 1.1, 'ventilation-unit': .85,
         'tower-stepped': 65., 'tower-crowned': 80., 'staircase': 5.,
         'catwalk': 8., 'rooftop-equipment': 2.5,
         'utility-room': 4., 'residential-midrise': 36., 'office-midrise': 48.}

def prepare(short):
    name = 'kairox-' + short
    folder = ROOT/'artifacts/model-jobs'/name
    source = folder/'model.glb'
    if not source.exists():
        print('Pending: '+name); return
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    points = [o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    lo = Vector(tuple(min(p[i] for p in points) for i in range(3)))
    hi = Vector(tuple(max(p[i] for p in points) for i in range(3)))
    origin = Vector(((lo.x+hi.x)/2, (lo.y+hi.y)/2, lo.z))
    extent = max(hi.x-lo.x,hi.y-lo.y) if short == 'catwalk' else hi.z-lo.z
    scale = SPECS[short]/extent
    for obj in meshes:
        matrix = obj.matrix_world.copy()
        for vertex in obj.data.vertices: vertex.co = (matrix @ vertex.co-origin)*scale
        obj.matrix_world.identity(); obj.data.update(); obj.name = name
    # Fit the replacement inside the existing service assembly envelope. The
    # provider's longest ground axis becomes game Z; roof stays exactly at 4m.
    if short == 'utility-room':
        from mathutils import Matrix
        import math
        rotation = Matrix.Rotation(math.pi*.5 if dimensions_x(meshes)>dimensions_y(meshes) else 0., 4, 'Z')
        for obj in meshes:
            obj.data.transform(rotation)
        points = [v.co for o in meshes for v in o.data.vertices]
        extent3 = Vector(tuple(max(p[i] for p in points)-min(p[i] for p in points) for i in range(3)))
        fit = Matrix.Diagonal((6./extent3.x, 10./extent3.y, 4./extent3.z, 1.))
        for obj in meshes:
            obj.data.transform(fit)
            obj.data.update()
    # New generations have a strict delivered budget, even if the provider
    # treats face_limit as a target. Keep UVs/materials; derive runtime LODs later.
    if short in {'utility-room','residential-midrise','office-midrise'}:
        faces=sum(len(o.data.polygons) for o in meshes)
        if faces>20000:
            for obj in meshes:
                bpy.context.view_layer.objects.active=obj
                modifier=obj.modifiers.new('Browser triangle budget','DECIMATE')
                modifier.ratio=19500./faces
                bpy.ops.object.modifier_apply(modifier=modifier.name)
    resolution = 1024 if short.startswith('tower') or short.endswith('midrise') or short=='utility-room' else 512
    for image in bpy.data.images:
        if image.size[0] and image.size[1]: image.scale(resolution,resolution); image.pack()
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes: obj.select_set(True)
    dest = ROOT/'assets/runtime/models/city-kit'/name
    dest.mkdir(parents=True,exist_ok=True)
    output = dest/(name+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,
        export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    raw=output.read_bytes(); size=struct.unpack_from('<I',raw,12)[0]
    doc=json.loads(raw[20:20+size]); binary=raw[28+size:]
    triangles=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
    points=[v.co for o in meshes for v in o.data.vertices]
    dimensions=Vector(tuple(max(p[i] for p in points)-min(p[i] for p in points) for i in range(3)))
    if short in {'utility-room','residential-midrise','office-midrise'}:
        assert triangles<=20000, 'Delivered runtime triangle budget exceeded'
    job=json.loads((folder/'model.glb.provenance.json').read_text())
    ref=json.loads((ROOT/'artifacts/model-input-images'/name/'provenance-v1.json').read_text(encoding='utf-8-sig'))
    textures=[]
    for im in doc.get('images',[]):
        view=doc['bufferViews'][im['bufferView']]; offset=view.get('byteOffset',0)
        data=binary[offset:offset+view['byteLength']]
        assert data[:8]==b'\x89PNG\r\n\x1a\n'
        textures.append(list(struct.unpack_from('>II',data,16)))
    assert all(d==[resolution,resolution] for d in textures)
    record=dict(asset=name,provider='Tripo3D',task_id=job['task_id'],credits_consumed=job['credits_consumed'],
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),reference_sha256=ref['sha256'],
        sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),triangles=triangles,
        parts=sum(len(m['primitives']) for m in doc['meshes']),materials=len(doc['materials']),
        texture_sizes=textures,dimensions_xyz_metres=[dimensions.x,dimensions.z,dimensions.y],
        coordinates='+Y up, bottom-centred origin; utility room fitted to 6x4x10m, other models use uniform metre scale; facade orientation is asset-specific',
        preparation='scripts/prepare-city-kit.py; metre normalization, reduced embedded maps, new models capped at 20000 triangles; shared runtime LODs',
        collision='Decorative placement outside track barriers; no road collision',
        license_status=job['license_status'])
    (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
    print('Prepared '+name+': '+str(triangles)+' triangles')

def dimensions_x(meshes):
    values=[v.co.x for o in meshes for v in o.data.vertices]
    return max(values)-min(values)

def dimensions_y(meshes):
    values=[v.co.y for o in meshes for v in o.data.vertices]
    return max(values)-min(values)

if __name__=='__main__':
    selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    for short in selected or SPECS: prepare(short.removeprefix('kairox-'))
    records=[]
    for short in SPECS:
        name='kairox-'+short
        path=ROOT/'assets/runtime/models/city-kit'/name/(name+'.asset.json')
        if not path.exists(): break
        records.append(json.loads(path.read_text()))
    if len(records)==len(SPECS):
        lines=['#pragma once','#include "engine/math.hpp"','#include <array>',
            'namespace racing::citykit {',
            'struct Model { const char* name; const char* hash; unsigned triangles,parts,materials; engine::Vec3 dimensions; };',
            'inline const std::array<Model,%d> models{{'%len(SPECS)]
        for r in records:
            dims=','.join(f'{x:.6f}f' for x in r['dimensions_xyz_metres'])
            lines.append('{"%s","%s",%d,%d,%d,{%s}},'%(r['asset'],r['sha256'],r['triangles'],r['parts'],r['materials'],dims))
        lines+=['}};','struct Placement { unsigned model; engine::Vec3 position; float yaw; float radius; float scale=1.f; };','}']
        (ROOT/'src/city_kit_contract.hpp').write_text('\n'.join(lines)+'\n')
