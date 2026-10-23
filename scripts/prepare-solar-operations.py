"""Normalize the retained Tripo operations house; Blender background, no API calls."""
import bpy,hashlib,json,math,struct,re
from pathlib import Path
from mathutils import Vector,Matrix
root=Path(__file__).resolve().parents[1]
source=root/'artifacts/model-jobs/solar-operations/model.glb'
dest=root/'assets/runtime/models/desert-solar/solar-operations';dest.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
for o in objects:o.matrix_world=Matrix.Rotation(math.pi/2,4,'Z')@o.matrix_world
points=[o.matrix_world@v.co for o in objects for v in o.data.vertices]
lo=Vector([min(p[i] for p in points) for i in range(3)]);hi=Vector([max(p[i] for p in points) for i in range(3)])
origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z));scale=11/max(hi.x-lo.x,hi.y-lo.y)
for o in objects:
 matrix=o.matrix_world.copy()
 for v in o.data.vertices:v.co=(matrix@v.co-origin)*scale
 o.matrix_world.identity();o.data.update()
for im in bpy.data.images:
 if im.type=='IMAGE' and max(im.size)>2048:
  w,h=im.size;im.scale(round(w*2048/max(w,h)),round(h*2048/max(w,h)))
for mat in bpy.data.materials:
 if mat.use_nodes:
  p=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
  p.inputs['Metallic'].default_value=0;p.inputs['Roughness'].default_value=.8
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
output=dest/'solar-operations.glb'
bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,export_yup=True,export_animations=False,export_lights=False,export_cameras=False)
raw=output.read_bytes();doc=json.loads(raw[20:20+struct.unpack_from('<I',raw,12)[0]])
prims=[p for m in doc['meshes'] for p in m['primitives']]
triangles=sum(doc['accessors'][p['indices']]['count']//3 for p in prims);assert 0<triangles<=20000
positions=[doc['accessors'][p['attributes']['POSITION']] for p in prims]
dimensions=[max(a['max'][i] for a in positions)-min(a['min'][i] for a in positions) for i in range(3)]
provenance=json.loads(source.with_suffix('.glb.provenance.json').read_text())
record=dict(name='solar-operations',sha256=hashlib.sha256(raw).hexdigest(),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),triangles=triangles,materials=len(doc['materials']),dimensions=dimensions,bytes=len(raw),texture_cap=2048,provider='Tripo3D',task_id=provenance['task_id'],preparation='scripts/prepare-solar-operations.py; uniform metric scale and bottom-center pivot; embedded PBR maps')
(dest/'solar-operations.asset.json').write_text(json.dumps(record,indent=2)+'\n')
header=root/'src/desert_art_contract.hpp';text=header.read_text()
entry='    {"solar-operations","%s",%d,%d,{%s}},\n'%(record['sha256'],triangles,record['materials'],','.join('%.6ff'%d for d in dimensions))
text=re.sub(r'^    \{"solar-operations".*\n','',text,flags=re.M)
text=re.sub(r'array<Model,\d+>', 'array<Model,13>',text).replace('}};',entry+'}};');header.write_text(text)
qa=root/'artifacts/model-jobs/solar-operations/qa';qa.mkdir(exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(qa/'operations.blend'))
scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True
scene.render.resolution_x=1000;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.5,.5,.5);scene.view_settings.view_transform='Standard'
center=Vector((0,0,dimensions[1]/2))
for location,power in [((10,-15,18),2500),((-12,-4,10),1800)]:
 data=bpy.data.lights.new('Studio','AREA');data.energy=power;data.size=12;o=bpy.data.objects.new('Studio',data);scene.collection.objects.link(o);o.location=location;o.rotation_euler=(center-o.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('QA');cam=bpy.data.objects.new('QA',data);scene.collection.objects.link(cam);scene.camera=cam;data.type='ORTHO';data.ortho_scale=17
for name,pos in [('front',(12,-16,11)),('back',(-12,16,11))]:
 cam.location=pos;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(qa/(name+'.png'));bpy.ops.render.render(write_still=True)
print(json.dumps(record))
