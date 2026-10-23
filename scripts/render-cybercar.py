"""Offline rear/profile QA in Blender; arguments after --: GLB output-directory [clay]."""
import sys
from pathlib import Path
import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
source, output = Path(args[0]).resolve(), Path(args[1]).resolve()
output.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(source))
if 'clay' in args:
    material = bpy.data.materials.new('Neutral diagnostic clay')
    material.diffuse_color = (.32, .36, .4, 1)
    material.use_nodes = True
    material.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (.32,.36,.4,1)
    material.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = .8
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            obj.data.materials.clear()
            obj.data.materials.append(material)
scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE'
scene.eevee.use_gtao = True
scene.eevee.gtao_distance = .18
scene.eevee.gtao_factor = 1.2
scene.eevee.taa_render_samples = 64
scene.world.color = (.22,.22,.22)
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'Medium High Contrast'
scene.view_settings.exposure = 0
scene.view_settings.gamma = 1
scene.render.resolution_x = 1000
scene.render.resolution_y = 850
scene.render.resolution_percentage = 100
for name, pos, power, size in [('Key',(0,-1,2),18,2),('Fill',(0,1,1),9.5,1.5),('Rim',(-1,0,1.5),12,1)]:
    data = bpy.data.lights.new(name, 'AREA')
    data.energy, data.size = power, size
    obj = bpy.data.objects.new(name,data)
    scene.collection.objects.link(obj)
    obj.location = pos
    obj.rotation_euler = (Vector((0,0,.12))-obj.location).to_track_quat('-Z','Y').to_euler()
camera = bpy.data.objects.new('QA camera', bpy.data.cameras.new('QA camera'))
scene.collection.objects.link(camera)
scene.camera = camera
camera.data.type = 'ORTHO'
for name, pos, scale in [('rear',(1.6,0,1.12),1.08),('rear-quarter',(1.3,-1.1,.8),1.28),('side',(0,-2,.55),1.3)]:
    camera.location = pos
    camera.rotation_euler = (Vector((0,0,.13))-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.ortho_scale = scale
    scene.render.filepath = str(output/(name+'.png'))
    bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(output/'inspection.blend'))
