"""Offline preparation of the two Tripo race infrastructure models. Blender 3.5."""
import bpy,hashlib,json,math,struct
import numpy as np
from pathlib import Path
from mathutils import Vector,Matrix
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[1]
records=[]
for name,limit,cap in [('desert-race-divider',3500,1024),('desert-race-gate',12000,2048)]:
 source=ROOT/'artifacts/model-jobs'/name/'model.glb';dest=ROOT/'assets/runtime/models/desert-race-kit'/name
 qa=source.parent/'qa';dest.mkdir(parents=True,exist_ok=True);qa.mkdir(exist_ok=True)
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=str(source))
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert meshes
 bpy.ops.object.select_all(action='DESELECT')
 for o in meshes:o.select_set(True)
 bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();obj=bpy.context.object
 bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);obj.name=name
 def bounds():return (Vector([min(v.co[i] for v in obj.data.vertices) for i in range(3)]),Vector([max(v.co[i] for v in obj.data.vertices) for i in range(3)]))
 obj.data.calc_loop_triangles();source_tris=len(obj.data.loop_triangles);assert source_tris<=20000,(name,source_tris)
 lo,hi=bounds();source_size=list(hi-lo)
 if hi.y-lo.y>hi.x-lo.x:
  for v in obj.data.vertices:v.co=Matrix.Rotation(math.pi/2,4,'Z')@v.co
 lo,hi=bounds();origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z));size=hi-lo
 # Divider dimensions deliberately match the already authoritative crash envelope.
 scale=Vector((4/size.x,.65/size.y,1.2/size.z)) if name.endswith('divider') else Vector((28/size.x,)*3)
 for v in obj.data.vertices:v.co=Vector([(v.co[i]-origin[i])*scale[i] for i in range(3)])
 if source_tris>limit:
  mod=obj.modifiers.new('Runtime triangle budget','DECIMATE');mod.ratio=(limit-20)/source_tris
  mod.use_collapse_triangulate=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 obj.data.update();obj.data.calc_loop_triangles();triangles=len(obj.data.loop_triangles)
 assert triangles<=limit;lo,hi=bounds()
 assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
 if name.endswith('gate'):
  tree=BVHTree.FromPolygons([v.co for v in obj.data.vertices],[tuple(t.vertices) for t in obj.data.loop_triangles],all_triangles=True)
  for x in np.linspace(-10.8,10.8,89):
   for z in np.linspace(.05,4.5,19):
    assert tree.ray_cast(Vector((float(x),lo.y-1,float(z))),Vector((0,1,0)),hi.y-lo.y+2)[0] is None,('Blocked gate opening',x,z)
 source_maps=[{'name':im.name,'size':list(im.size)} for im in bpy.data.images if im.type=='IMAGE']
 for im in list(bpy.data.images):
  if im.type!='IMAGE':continue
  w,h=im.size
  if max(w,h)>cap:im.scale(max(1,round(w*cap/max(w,h))),max(1,round(h*cap/max(w,h))))
  im.pack()
 emission_pixels=0
 for mat in obj.data.materials:
  bsdf=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED');links=mat.node_tree.links
  image=bsdf.inputs['Base Color'].links[0].from_node.image;w,h=image.size
  pixels=np.empty(w*h*4,dtype=np.float32);image.pixels.foreach_get(pixels);pixels=pixels.reshape((-1,4))
  r,g,b=pixels[:,0],pixels[:,1],pixels[:,2];mask=(g>r*1.3)&(b>r*1.3)&(g>.12)&(b>.12)
  out=np.zeros_like(pixels);out[:,3]=1;out[mask,:3]=pixels[mask,:3];emission_pixels+=int(mask.sum())
  emission=bpy.data.images.new(name+'-cyan-emission',width=w,height=h,alpha=True);emission.pixels.foreach_set(out.reshape(-1));emission.pack()
  node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=emission;links.new(node.outputs['Color'],bsdf.inputs['Emission']);bsdf.inputs['Emission Strength'].default_value=.8
 output=dest/(name+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
 blob=output.read_bytes();doc=json.loads(blob[20:20+struct.unpack_from('<I',blob,12)[0]])
 actual=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives']);assert actual==triangles
 provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
 record={'name':name,'provider':'Tripo3D','task_id':provider['task_id'],'credits_consumed':provider['credits_consumed'],
  'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'reference_sha256':provider['image_sha256'],
  'sha256':hashlib.sha256(blob).hexdigest(),'bytes':len(blob),'source_triangles':source_tris,'triangles':triangles,
  'dimensions':[hi.x-lo.x,hi.z-lo.z,hi.y-lo.y],'materials':len(doc['materials']),'source_maps':source_maps,'texture_cap':cap,
  'source_size_blender':source_size,'scale_xyz_blender':list(scale),'emission_pixels':emission_pixels,
  'preparation':'Bottom centre pivot, +Y up, long axis X, budgeted decimation with retained UV maps, reduced embedded PBR maps and cyan-only emission. Divider fitted to existing 4 x 1.2 x 0.65m physics envelope; gate uniformly scaled to 28m width.',
  'qa':'Finite mesh, source <=20000 triangles, prepared budget, gate clear corridor 21.6m x 4.5m sampled by mesh rays, front and reverse renders. Runtime acceptance recorded separately.',
  'license_status':provider['license_status']}
 (dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n');records.append(record)
 bpy.ops.wm.save_as_mainfile(filepath=str(qa/(name+'.blend')))
 scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True;scene.eevee.gtao_distance=1.5;scene.eevee.taa_render_samples=32
 scene.render.resolution_x=1200;scene.render.resolution_y=600;scene.render.resolution_percentage=100
 scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.2,.24,.28,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.6
 scene.view_settings.view_transform='Standard';scene.view_settings.look='Medium High Contrast'
 center=(lo+hi)/2;extent=max(hi-lo)
 for label,direction,power in [('Key',(1,-2,3),70),('Fill',(-2,-1,2),45),('Rim',(1,2,2),60)]:
  light=bpy.data.lights.new(label,'AREA');light.energy=power*extent*extent;light.size=extent*2
  ob=bpy.data.objects.new(label,light);scene.collection.objects.link(ob);ob.location=center+Vector(direction)*extent;ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
 camdata=bpy.data.cameras.new('Review');cam=bpy.data.objects.new('Review',camdata);scene.collection.objects.link(cam);scene.camera=cam;camdata.type='ORTHO';camdata.ortho_scale=extent*1.2
 for label,direction in [('front',(.3,-2,.65)),('back',(-.3,2,.65))]:
  cam.location=center+Vector(direction)*extent;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(qa/(label+'.png'));bpy.ops.render.render(write_still=True)
 print('RACE_KIT_ASSET '+json.dumps(record),flush=True)
header=['#pragma once','#include "engine/math.hpp"','#include <array>','namespace racing::desertrace {','struct Model {const char* name;const char* hash;unsigned triangles,materials;engine::Vec3 dimensions;};','inline constexpr std::array<Model,2> models{{']
for r in records:header.append('    {"%s","%s",%d,%d,{%s}},'%(r['name'],r['sha256'],r['triangles'],r['materials'],','.join('%.6ff'%v for v in r['dimensions'])))
header+=['}};','}'];(ROOT/'src/desert_race_contract.hpp').write_text('\n'.join(header)+'\n')
(ROOT/'assets/runtime/models/desert-race-kit/manifest.json').write_text(json.dumps({'assets':records,'total_bytes':sum(r['bytes'] for r in records),'credits_consumed':sum(r['credits_consumed'] for r in records)},indent=2)+'\n')
