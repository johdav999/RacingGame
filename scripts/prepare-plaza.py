"""Offline Blender normalization and reduction for the retained Tripo plaza kit."""
import bpy, json, hashlib, struct
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
records=[]
for short,target,tri_target in [('paving',(4.,4.,.12),3500),('drain',(.45,4.,.15),6000)]:
    name='kairox-plaza-'+short
    source=ROOT/'artifacts/model-jobs'/name/'model.glb'
    if not source.exists(): print('Pending '+name);continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    for o in meshes:
        matrix=o.matrix_world.copy()
        for v in o.data.vertices:v.co=matrix@v.co
        o.matrix_world.identity()
    points=[v.co.copy() for o in meshes for v in o.data.vertices]
    lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    # All modules run along local glTF Z, with radial width on X.
    if short=='drain' and hi.x-lo.x>hi.y-lo.y:
        for o in meshes:
            for v in o.data.vertices:v.co=Vector((-v.co.y,v.co.x,v.co.z))
        points=[v.co.copy() for o in meshes for v in o.data.vertices]
        lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
        hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    source_triangles=0
    for o in meshes:
        o.data.calc_loop_triangles();source_triangles+=len(o.data.loop_triangles)
        for v in o.data.vertices:
            v.co=Vector(tuple((v.co[i]-origin[i])*target[i]/(hi[i]-lo[i]) for i in range(3)))
        o.data.update()
        bpy.context.view_layer.objects.active=o
        mod=o.modifiers.new('Browser geometry budget','DECIMATE');mod.ratio=min(1,tri_target/max(1,source_triangles))
        mod.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=mod.name)
        o.name=name
    # Collapse may move silhouette extrema slightly. Restore the exact connector
    # bounds so neighbouring modules meet and puddle planes stay above the top.
    points=[v.co.copy() for o in meshes for v in o.data.vertices]
    lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    for o in meshes:
        for v in o.data.vertices:
            v.co=Vector(tuple((v.co[i]-lo[i])*target[i]/(hi[i]-lo[i])-(target[i]/2 if i<2 else 0) for i in range(3)))
        o.data.update()
    for im in bpy.data.images:
        if im.size[0] and im.size[1]:im.scale(1024,1024);im.pack()
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:o.select_set(True)
    dest=ROOT/'assets/runtime/models/plaza'/name;dest.mkdir(parents=True,exist_ok=True)
    output=dest/(name+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,
        export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    raw=output.read_bytes();size=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+size])
    accessors=[doc['accessors'][p['attributes']['POSITION']] for m in doc['meshes'] for p in m['primitives']]
    lower=[min(a['min'][i] for a in accessors) for i in range(3)]
    upper=[max(a['max'][i] for a in accessors) for i in range(3)]
    job=json.loads(source.with_suffix('.glb.provenance.json').read_text())
    reference=json.loads((ROOT/'artifacts/model-input-images'/name/'provenance-v1.json').read_text())
    record=dict(asset=name,provider='Tripo3D',task_id=job['task_id'],credits_consumed=job['credits_consumed'],
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),reference_sha256=reference['sha256'],
        sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),source_triangles=source_triangles,
        triangles=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives']),
        bounds_min=lower,bounds_max=upper,dimensions_xyz_metres=[upper[i]-lower[i] for i in range(3)],
        preparation='scripts/prepare-plaza.py; fitted module dimensions, decimated mesh, embedded 1024px maps',
        coordinates='+Y up, longitudinal +Z, bottom-centred; runtime conforms module to curved plaza',
        license_status=job['license_status'])
    (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n');records.append(record)
    print(json.dumps(record))
if len(records)==2:
    lines=['#pragma once','#include "engine/math.hpp"','#include <array>','namespace racing::plaza {',
        'struct Model {const char* name;const char* hash;unsigned triangles;engine::Vec3 dimensions;};',
        'inline const std::array<Model,2> models{{']
    for r in records:
        lines.append('{"%s","%s",%d,{%s}},'%(r['asset'],r['sha256'],r['triangles'],','.join(f'{x:.6f}f' for x in r['dimensions_xyz_metres'])))
    lines+=['}};','}'];(ROOT/'src/plaza_art_contract.hpp').write_text('\n'.join(lines)+'\n')
