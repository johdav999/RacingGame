"""Offline multi-view asset QA. Blender --background --python this.py -- source.glb output-dir."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
source,folder=map(Path,sys.argv[sys.argv.index('--')+1:])
folder.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source.resolve()))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
lo=Vector(tuple(min(p[i] for p in points) for i in range(3)));hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
center=(lo+hi)/2;extent=max(hi-lo)
scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True
scene.eevee.taa_render_samples=24;scene.render.resolution_x=800;scene.render.resolution_y=600;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.4,.4,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.65
scene.view_settings.view_transform='Standard'
for name,pos,power in [('Key',(1,-2,3),100),('Fill',(-2,-1,1),60),('Rim',(1,2,2),90)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power*extent*extent;data.size=extent*2
    o=bpy.data.objects.new(name,data);scene.collection.objects.link(o);o.location=center+Vector(pos)*extent
    o.rotation_euler=(center-o.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('Camera');camera=bpy.data.objects.new('Camera',data);scene.collection.objects.link(camera)
scene.camera=camera;data.type='ORTHO';data.ortho_scale=extent*1.4
for name,direction in [('positive-x',(1.7,-1.7,1)),('negative-x',(-1.7,1.7,1)),('side',(0,-2,.4)),('under',(1,-1,-1))]:
    camera.location=center+Vector(direction)*extent;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    scene.render.filepath=str((folder/(name+'.png')).resolve());bpy.ops.render.render(write_still=True)
for o in meshes:o.data.calc_loop_triangles()
record=dict(bounds_blender=[list(lo),list(hi)],triangles=sum(len(o.data.loop_triangles) for o in meshes),materials=len(bpy.data.materials),images=[dict(name=i.name,size=list(i.size)) for i in bpy.data.images if i.source=='FILE'])
(folder/'qa.json').write_text(json.dumps(record,indent=2));print(json.dumps(record))
