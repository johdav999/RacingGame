"""Prepare and render the approved Tripo desert kit; preserves provider originals.
Run Blender --background --python scripts/prepare-desert-solar.py.
"""
import bpy, hashlib, json, math, struct, sys
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix

ROOT=Path(__file__).resolve().parents[1]
specs=json.loads((ROOT/'docs/desert-solar-prompts.json').read_text())['assets']
# Uniform scaling preserves provider proportions. Values are primary dimensions in metres.
targets={'solar-mirror-intact':('width',6),'solar-mirror-broken':('width',6),
 'solar-receiver-tower':('height',55),'sandstone-cliff-wall':('width',20),
 'sandstone-cliff-corner':('width',12),'sandstone-cliff-buttress':('height',14),
 'sandstone-cliff-low-shelf':('width',16),'sandstone-rock-arch':('width',24),
 'energy-conduit-straight':('width',4),'energy-conduit-elbow':('height',.9),
 'energy-conduit-junction':('height',.9),'energy-conduit-support':('height',1.2)}
selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for spec in specs:
 name=spec['id']
 if selected and name not in selected: continue
 source=ROOT/'artifacts/model-jobs/desert-solar'/name/'model.glb'
 dest=ROOT/'assets/runtime/models/desert-solar'/name
 qa=ROOT/'artifacts/model-jobs/desert-solar'/name/'qa'
 dest.mkdir(parents=True,exist_ok=True);qa.mkdir(exist_ok=True)
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.gltf(filepath=str(source))
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
 assert meshes, name
 bpy.ops.object.select_all(action='DESELECT')
 for o in meshes:o.select_set(True)
 bpy.context.view_layer.objects.active=meshes[0]
 bpy.ops.object.join();obj=bpy.context.object
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 obj.name=name
 def bounds():
  return (Vector(tuple(min(v.co[i] for v in obj.data.vertices) for i in range(3))),Vector(tuple(max(v.co[i] for v in obj.data.vertices) for i in range(3))))
 lo,hi=bounds();raw_dims=list(hi-lo)
 obj.data.calc_loop_triangles();tris=len(obj.data.loop_triangles)
 assert tris<=spec['budget']<=20000,(name,tris,spec['budget'])
 mode,target=targets[name]
 # Align broad assets along X, retaining up and depth.
 if mode=='width' and hi.y-lo.y>hi.x-lo.x:
  rot=Matrix.Rotation(math.pi/2,4,'Z')
  for v in obj.data.vertices:v.co=rot@v.co
  lo,hi=bounds()
 scale=target/(hi.z-lo.z if mode=='height' else hi.x-lo.x)
 origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
 for v in obj.data.vertices:v.co=(v.co-origin)*scale
 obj.data.update();lo,hi=bounds()
 assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
 source_textures=[{'name':im.name,'size':list(im.size)} for im in bpy.data.images if im.type=='IMAGE']
 cap=1024 if ('conduit' in name or 'mirror' in name) else 2048
 for im in list(bpy.data.images):
  if im.type!='IMAGE':continue
  w,h=im.size
  if max(w,h)>cap:im.scale(max(1,round(w*cap/max(w,h))),max(1,round(h*cap/max(w,h))))
  im.pack()
 if name=='solar-mirror-intact':
  # Provider mirror normal/roughness reconstruction causes large jagged highlights.
  # Identify blue mirror faces by their UV-sampled base colour, retain the frame,
  # and give the mirror regions a consistent optical normal and satin-metal finish.
  original=obj.data.materials[0];glass=original.copy();glass.name='solar-mirror-glass'
  nodes=glass.node_tree.nodes;links=glass.node_tree.links
  bsdf=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
  for label,value in [('Roughness',.32),('Metallic',.9)]:
   for link in list(bsdf.inputs[label].links):links.remove(link)
   bsdf.inputs[label].default_value=value
  for link in list(bsdf.inputs['Normal'].links):links.remove(link)
  obj.data.materials.append(glass);index=len(obj.data.materials)-1
  tex=bsdf.inputs['Base Color'].links[0].from_node.image;w,h=tex.size
  values=np.empty(w*h*4,dtype=np.float32);tex.pixels.foreach_get(values);values=values.reshape((h,w,4))
  uv=obj.data.uv_layers.active.data;glass_faces=[]
  for face in obj.data.polygons:
   coord=sum((uv[i].uv for i in face.loop_indices),Vector((0,0)))/len(face.loop_indices)
   r,g,b,a=values[min(h-1,int(coord.y*h)%h),min(w-1,int(coord.x*w)%w)]
   if b>r*1.16 and b>g*1.04 and b>.05:
    face.material_index=index;glass_faces.append(face)
  normal=sum((p.normal*p.area for p in glass_faces),Vector((0,0,0))).normalized()
  obj.data.calc_normals_split();normals=[tuple(l.normal) for l in obj.data.loops]
  for face in glass_faces:
   for i in face.loop_indices:normals[i]=tuple(normal)
  obj.data.use_auto_smooth=True;obj.data.normals_split_custom_set(normals)
 emission_pixels=0
 if 'conduit' in name or 'receiver' in name:
  for mat in obj.data.materials:
   if not mat.use_nodes:continue
   nodes=mat.node_tree.nodes;links=mat.node_tree.links
   bsdf=next((n for n in nodes if n.type=='BSDF_PRINCIPLED'),None)
   if not bsdf or not bsdf.inputs['Base Color'].is_linked:continue
   tex=bsdf.inputs['Base Color'].links[0].from_node
   if tex.type!='TEX_IMAGE':continue
   im=tex.image;w,h=im.size
   pixels=np.empty(w*h*4,dtype=np.float32);im.pixels.foreach_get(pixels)
   pixels=pixels.reshape((-1,4));r,g,b=pixels[:,0],pixels[:,1],pixels[:,2]
   # Extract only saturated cyan from provider base color into an emissive map.
   mask=(g>r*1.35)&(b>r*1.35)&(g>.12)&(b>.12)
   emission_pixels+=int(mask.sum())
   out=np.zeros_like(pixels);out[:,3]=1;out[mask,:3]=pixels[mask,:3]
   emission=bpy.data.images.new(name+'-cyan-emission',width=w,height=h,alpha=True)
   emission.pixels.foreach_set(out.reshape(-1));emission.pack()
   node=nodes.new('ShaderNodeTexImage');node.image=emission
   links.new(node.outputs['Color'],bsdf.inputs['Emission']);bsdf.inputs['Emission Strength'].default_value=1
 output=dest/(name+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
 data=output.read_bytes();doc=json.loads(data[20:20+struct.unpack_from('<I',data,12)[0]])
 exported=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
 assert exported==tris
 provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
 record={'asset':name,'provider':'Tripo3D','task_id':provider['task_id'],'credits_consumed':provider.get('credits_consumed'),
 'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'reference_sha256':provider['image_sha256'],
 'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'triangles':exported,'requested_triangle_limit':spec['budget'],
 'dimensions_xyz_metres':[hi.x-lo.x,hi.z-lo.z,hi.y-lo.y],'coordinates':'glTF +Y up, bottom centre origin',
 'scale_factor':scale,'source_dimensions_blender':raw_dims,'source_textures':source_textures,'texture_dimension_cap':cap,
 'emission_mask_pixels':emission_pixels,'materials':len(doc.get('materials',[])),
 'preparation':'Uniform metric scale, bottom-centre pivot, texture reduction; cyan-only emissive map for tower and conduits; intact mirror optical normals/material corrected. Provider geometry retained.',
 'qa_scope':'Blender import, finite geometry, triangle counts, embedded maps, two rendered views. Not integrated or performance-tested in game.',
 'license_status':provider['license_status']}
 (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
 (qa/'qa.json').write_text(json.dumps(record,indent=2)+'\n')
 obj.select_set(True)
 bpy.ops.wm.save_as_mainfile(filepath=str(qa/(name+'.blend')))
 center=(lo+hi)/2;extent=max(hi-lo)
 scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True
 scene.eevee.gtao_distance=extent*.1;scene.eevee.taa_render_samples=32
 scene.render.resolution_x=720;scene.render.resolution_y=720;scene.render.resolution_percentage=100
 scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True
 scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.23,.26,.30,1)
 scene.world.node_tree.nodes['Background'].inputs[1].default_value=.65
 scene.view_settings.view_transform='Standard';scene.view_settings.look='Medium High Contrast'
 for label,direction,power in [('Key',(1,-2,3),100),('Fill',(-2,-1,1.5),60),('Rim',(1,2,2),90)]:
  light=bpy.data.lights.new(label,'AREA');light.energy=power*extent*extent;light.size=extent*2
  ob=bpy.data.objects.new(label,light);scene.collection.objects.link(ob)
  ob.location=center+Vector(direction)*extent;ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
 camdata=bpy.data.cameras.new('Review');cam=bpy.data.objects.new('Review',camdata)
 scene.collection.objects.link(cam);scene.camera=cam;camdata.type='ORTHO';camdata.ortho_scale=extent*1.45
 for label,direction in [('front',(1.3,-1.6,.85)),('back',(-1.3,1.6,.85))]:
  cam.location=center+Vector(direction)*extent;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
  scene.render.filepath=str(qa/(label+'.png'));bpy.ops.render.render(write_still=True)
 print('DESERT_ASSET '+json.dumps(record),flush=True)
