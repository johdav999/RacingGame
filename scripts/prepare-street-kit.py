"""Offline normalization and four-view QA of retained Tripo street kit jobs.

blender --background --python scripts/prepare-street-kit.py -- <short-name> ...
No provider calls. Originals and their UVs/geometry remain intact.
"""
import bpy
import hashlib
import json
import math
import sys
from pathlib import Path
from mathutils import Vector, Matrix

ROOT=Path(__file__).resolve().parents[1]
# Target axis is in Blender coordinates (Z up). All exports use glTF Y up.
SPECS={
    'transit-pillar':(2,8,1024), 'transit-beam':(0,8,1024),
    'transit-deck':(0,8,1024), 'station':(2,17.6,1024),
    'shop-shutter':(0,4,512), 'shop-doorway':(2,3,512),
    'shop-awning':(0,4,512), 'shop-sign':(0,4,512),
    'safety-fence':(0,4,512), 'spectator-railing':(0,4,512),
    'charging-pedestal':(2,1.8,512), 'utility-cabinet':(2,1.6,512)}

def prepare(short):
    axis,target,texsize=SPECS[short]
    name='kairox-'+short
    source=ROOT/'artifacts/model-jobs'/(name+('-v1' if short=='transit-pillar' else ''))/'model.glb'
    dest=ROOT/'assets/runtime/models/street-kit'/name
    qa=source.parent/'runtime-qa'
    dest.mkdir(parents=True,exist_ok=True);qa.mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert meshes,'Missing model geometry'
    # Provider front is +X; canonical facade front is glTF +Z (Blender -Y).
    turn=Matrix.Rotation(-math.pi/2,4,'Z')
    for obj in meshes:
        matrix=turn @ obj.matrix_world
        for vertex in obj.data.vertices: vertex.co=matrix @ vertex.co
        obj.matrix_world.identity()
    def bounds():
        points=[v.co for o in meshes for v in o.data.vertices]
        return Vector(tuple(min(p[i] for p in points) for i in range(3))),Vector(tuple(max(p[i] for p in points) for i in range(3)))
    lo,hi=bounds()
    # Horizontal structural modules join along X, regardless of provider yaw.
    if short in ('transit-beam','transit-deck') and hi.x-lo.x<hi.y-lo.y:
        turn=Matrix.Rotation(math.pi/2,4,'Z')
        for obj in meshes:
            for vertex in obj.data.vertices: vertex.co=turn @ vertex.co
        lo,hi=bounds()
    scale=target/(hi[axis]-lo[axis])
    origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    triangles=0
    for obj in meshes:
        for vertex in obj.data.vertices: vertex.co=(vertex.co-origin)*scale
        obj.data.update();obj.data.calc_loop_triangles()
        triangles+=len(obj.data.loop_triangles)
        obj.name=name
    for im in bpy.data.images:
        if im.size[0] and im.size[1]: im.scale(texsize,texsize);im.pack()
    lo,hi=bounds();dimensions=hi-lo
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes: obj.select_set(True)
    output=dest/(name+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,
        export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
    provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
    record={'asset':name,'provider':'Tripo3D','task_id':provider['task_id'],
        'credits_consumed':provider['credits_consumed'],'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
        'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'reference_sha256':provider['image_sha256'],
        'bytes':output.stat().st_size,'triangles':triangles,'texture_size':texsize,
        'dimensions':[dimensions.x,dimensions.z,dimensions.y],
        'coordinates':'+Y up, bottom-centre origin, facade front +Z, structural length +X',
        'preparation':'scripts/prepare-street-kit.py; uniform metric scale, preserved geometry and UVs, resized embedded maps',
        'license_status':provider['license_status']}
    (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
    (qa/'qa.json').write_text(json.dumps(record,indent=2)+'\n')
    scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE'
    scene.eevee.use_gtao=True;scene.eevee.taa_render_samples=24
    scene.render.resolution_x=800;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.32,.36,.42,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    center=(lo+hi)/2;extent=max(dimensions)
    for label,direction,power in [('Key',(1,-2,3),350),('Fill',(-2,-1,1.5),180),('Rim',(1,2,2),260)]:
        data=bpy.data.lights.new(label,'AREA');data.energy=power*extent*extent*.35;data.size=extent*2
        obj=bpy.data.objects.new(label,data);scene.collection.objects.link(obj)
        obj.location=center+Vector(direction)*extent
        obj.rotation_euler=(center-obj.location).to_track_quat('-Z','Y').to_euler()
    data=bpy.data.cameras.new('QA');camera=bpy.data.objects.new('QA',data);scene.collection.objects.link(camera)
    scene.camera=camera;data.type='ORTHO';data.ortho_scale=extent*1.65
    for label,direction in {'front':(1.3,-1.6,1.05),'rear':(-1.3,1.6,.9),'side':(2,0,.4),'top':(.001,0,2)}.items():
        camera.location=center+Vector(direction)*extent
        camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(qa/(label+'.png'));bpy.ops.render.render(write_still=True)
    print(json.dumps(record),flush=True)

for short in sys.argv[sys.argv.index('--')+1:]: prepare(short)
