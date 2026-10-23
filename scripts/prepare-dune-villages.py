"""Headless Blender QA/preparation of the five user-supplied dune buildings.

blender --background --python scripts/prepare-dune-villages.py
Original GLBs are retained untouched. No provider calls.
"""
import bpy
import hashlib
import json
import math
import struct
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
records = []
for index in range(1, 6):
    name = f'dune-building-{index}'
    source = ROOT / f'assets/runtime/models/buildings/DuneBuilding{index}.glb'
    dest = ROOT / 'assets/runtime/models/dune-villages' / name
    qa = ROOT / 'artifacts/model-jobs/dune-villages' / name
    dest.mkdir(parents=True, exist_ok=True)
    qa.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    assert meshes
    points = [o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    assert all(math.isfinite(c) for p in points for c in p)
    lo = Vector([min(p[i] for p in points) for i in range(3)])
    hi = Vector([max(p[i] for p in points) for i in range(3)])
    original_size = list(hi-lo)
    origin = Vector(((lo.x+hi.x)/2, (lo.y+hi.y)/2, lo.z))
    scale = 16 / max(hi.x-lo.x, hi.y-lo.y)
    triangles = 0
    for obj in meshes:
        obj.data.calc_loop_triangles()
        triangles += len(obj.data.loop_triangles)
        assert obj.data.uv_layers, 'Missing texture coordinates'
        matrix = obj.matrix_world.copy()
        for v in obj.data.vertices:
            v.co = (matrix @ v.co-origin)*scale
        obj.matrix_world.identity()
        obj.data.update()
    assert 0 < triangles <= 20000
    maps = []
    for im in bpy.data.images:
        if im.type != 'IMAGE':
            continue
        w, h = im.size
        assert w and h and im.has_data
        maps.append({'name': im.name, 'source_size': [w,h]})
        if max(w,h)>1024:
            im.scale(round(w*1024/max(w,h)),round(h*1024/max(w,h)))
        im.pack()
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes:
        obj.select_set(True)
    output = dest / (name+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(output), export_format='GLB', use_selection=True,
        export_yup=True, export_animations=False, export_cameras=False, export_lights=False)
    blob = output.read_bytes()
    doc = json.loads(blob[20:20+struct.unpack_from('<I',blob,12)[0]])
    assert sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives']) == triangles
    dimensions = (hi-lo)*scale
    record = dict(name=name, source=source.relative_to(ROOT).as_posix(),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), sha256=hashlib.sha256(blob).hexdigest(),
        bytes=len(blob), triangles=triangles, materials=len(doc['materials']),
        dimensions=[dimensions.x,dimensions.z,dimensions.y], source_dimensions_blender=original_size,
        maps=maps, texture_cap=1024, provenance='User-supplied GLB; provider and license not independently verified',
        preparation='All source triangles and UVs retained; uniform metric scale; bottom-centre pivot; embedded 1024px maps',
        qa='Finite geometry, UVs, embedded images, triangle preservation, exported GLB re-import and front/reverse renders')
    (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
    records.append(record)
    # Inspect the exported artifact, not just the pre-export scene.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(output))
    bpy.ops.wm.save_as_mainfile(filepath=str(qa/(name+'.blend')))
    scene=bpy.context.scene
    scene.render.engine='BLENDER_EEVEE'
    scene.eevee.use_gtao=True
    scene.eevee.gtao_distance=3
    scene.eevee.taa_render_samples=32
    scene.render.resolution_x=900
    scene.render.resolution_y=900
    scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Studio')
    scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.25,.28,.32,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    scene.view_settings.view_transform='Standard'
    center=Vector((0,0,dimensions.z/2))
    extent=max(dimensions)
    for label,direction,power in [('Key',(1,-2,3),65),('Fill',(-2,-1,2),35),('Rim',(1,2,2),50)]:
        light=bpy.data.lights.new(label,'AREA');light.energy=power*extent*extent;light.size=extent*2
        ob=bpy.data.objects.new(label,light);scene.collection.objects.link(ob)
        ob.location=center+Vector(direction)*extent
        ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
    camdata=bpy.data.cameras.new('Review');cam=bpy.data.objects.new('Review',camdata)
    scene.collection.objects.link(cam);scene.camera=cam
    camdata.type='ORTHO';camdata.ortho_scale=extent*1.65
    for label,direction in [('front',(1,-2,1.1)),('back',(-1,2,1.1))]:
        cam.location=center+Vector(direction)*extent
        cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(qa/(label+'.png'))
        bpy.ops.render.render(write_still=True)
    print('DUNE_ASSET '+json.dumps(record),flush=True)

header=['#pragma once','#include "engine/math.hpp"','#include <array>',
    'namespace racing::dune {','struct Model {const char* name;const char* hash;unsigned triangles,materials;engine::Vec3 dimensions;};',
    'inline constexpr std::array<Model,5> models{{']
for r in records:
    header.append('    {"%s","%s",%d,%d,{%s}},'%(r['name'],r['sha256'],r['triangles'],r['materials'],','.join('%.6ff'%v for v in r['dimensions'])))
header+=['}};','}']
(ROOT/'src/dune_village_contract.hpp').write_text('\n'.join(header)+'\n')
(ROOT/'assets/runtime/models/dune-villages/manifest.json').write_text(json.dumps({'assets':records,'total_bytes':sum(r['bytes'] for r in records)},indent=2)+'\n')
