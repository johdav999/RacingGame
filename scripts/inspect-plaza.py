"""Render all prepared city models from three directions in Blender (offline)."""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for path in sorted((ROOT/'assets/runtime/models/plaza').glob('*/*.glb')):
    if selected and path.stem not in selected: continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    points=[o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    center=(lo+hi)/2;extent=max(hi-lo)
    scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE'
    scene.eevee.use_gtao=True;scene.eevee.gtao_distance=extent*.15
    scene.eevee.taa_render_samples=24
    scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.3,.34,.4,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    scene.view_settings.view_transform='Filmic'
    for name,direction,power in [('Key',(1,-2,3),180),('Fill',(-2,-1,1.5),90),('Rim',(1,2,2),130)]:
        data=bpy.data.lights.new(name,'AREA');data.energy=power*extent*extent;data.size=extent*2
        obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj)
        obj.location=center+Vector(direction)*extent
        obj.rotation_euler=(center-obj.location).to_track_quat('-Z','Y').to_euler()
    data=bpy.data.cameras.new('QA');camera=bpy.data.objects.new('QA',data)
    scene.collection.objects.link(camera);scene.camera=camera;data.type='ORTHO';data.ortho_scale=extent*1.5
    folder=ROOT/'artifacts/model-jobs'/path.stem/'runtime-qa';folder.mkdir(exist_ok=True)
    for name,direction in [('front',(1.3,-1.6,1.05)),('back',(-1.3,1.6,.9)),('side',(0,-2,.4))]:
        camera.location=center+Vector(direction)*extent
        camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(folder/(name+'.png'));bpy.ops.render.render(write_still=True)
    (folder/'qa.json').write_text(json.dumps(dict(asset=path.stem,bounds_blender=[list(lo),list(hi)],
        mesh_objects=len(meshes),materials=len(bpy.data.materials),views=['front','back','side']),indent=2))

