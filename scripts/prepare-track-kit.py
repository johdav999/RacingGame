"""Offline Tripo track kit reduction with fresh UVs and texture rebaking.

Run with Blender --background --python scripts/prepare-track-kit.py -- <asset>.
Retains provider originals; no paid calls. Runtime geometry is metric, +Y up.
"""
import bpy, bmesh, hashlib, json, math, struct, sys
from pathlib import Path
from mathutils import Vector, Matrix

ROOT=Path(__file__).resolve().parents[1]
specs=json.loads((ROOT/'docs/track-kit-spec.json').read_text())
name=sys.argv[sys.argv.index('--')+1]
spec=next(a for a in specs if a['name']==name)
if 'gantry' in name:spec['dimensions'][0]=35 # Includes broad provider feet: inner edge must clear both barriers.
source=ROOT/'artifacts/model-jobs'/name/'model.glb'
dest=ROOT/'assets/runtime/models/track-kit'/name
dest.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join();high=bpy.context.object
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
turn=Matrix.Rotation(-math.pi/2,4,'Z')
for v in high.data.vertices:v.co=turn @ v.co
def bounds(obj):
    return (Vector(tuple(min(v.co[i] for v in obj.data.vertices) for i in range(3))),
            Vector(tuple(max(v.co[i] for v in obj.data.vertices) for i in range(3))))
lo,hi=bounds(high)
dx,dy,dz=spec['dimensions'];target=Vector((dx,dz,dy))
if dx>dz*1.5 and hi.x-lo.x<hi.y-lo.y:
    turn=Matrix.Rotation(math.pi/2,4,'Z')
    for v in high.data.vertices:v.co=turn@v.co
    lo,hi=bounds(high)
origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z));size=hi-lo
for v in high.data.vertices:v.co=Vector(tuple((v.co[i]-origin[i])*target[i]/size[i] for i in range(3)))
high.data.update();high.data.calc_loop_triangles();source_triangles=len(high.data.loop_triangles)
high.name=name+'-source'
low=high.copy();low.data=high.data.copy();bpy.context.collection.objects.link(low)
low.name=name
bpy.ops.object.select_all(action='DESELECT');low.select_set(True);bpy.context.view_layer.objects.active=low
bm=bmesh.new();bm.from_mesh(low.data)
bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=max(target)*1e-6)
bm.to_mesh(low.data);bm.free()
dec=low.modifiers.new('Runtime simplification','DECIMATE');dec.ratio=spec['budget']/source_triangles
bpy.ops.object.modifier_apply(modifier=dec.name)
for layer in list(low.data.uv_layers):low.data.uv_layers.remove(layer)
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.018)
bpy.ops.object.mode_set(mode='OBJECT')
# Bake from original surfaces, rather than dragging provider UVs through collapse.
mat=bpy.data.materials.new(name);mat.use_nodes=True
low.data.materials.clear();low.data.materials.append(mat)
for p in low.data.polygons:p.material_index=0
nodes=mat.node_tree.nodes;links=mat.node_tree.links;bsdf=nodes.get('Principled BSDF')
texsize=1024 if 'gantry' in name else 512
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=1
scene.render.bake.use_selected_to_active=True;scene.render.bake.cage_extrusion=max(target)*.003
scene.render.bake.max_ray_distance=max(target)*.08;scene.render.bake.margin=8
original=[]
for m in high.data.materials:
    out=next(n for n in m.node_tree.nodes if n.type=='OUTPUT_MATERIAL')
    shader=out.inputs['Surface'].links[0].from_socket
    original.append((m,out,shader))
def source_channel(channel):
    for m,out,shader in original:
        n=m.node_tree.nodes;l=m.node_tree.links
        if channel=='normal':l.new(shader,out.inputs['Surface']);continue
        emit=n.new('ShaderNodeEmission')
        if channel=='base':
            socket=shader.node.inputs['Base Color']
            if socket.is_linked:l.new(socket.links[0].from_socket,emit.inputs['Color'])
            else:emit.inputs['Color'].default_value=socket.default_value
        else:
            combine=n.new('ShaderNodeCombineRGB');combine.inputs[0].default_value=1
            for label,i in [('Roughness',1),('Metallic',2)]:
                socket=shader.node.inputs[label]
                if socket.is_linked:l.new(socket.links[0].from_socket,combine.inputs[i])
                else:combine.inputs[i].default_value=socket.default_value
            l.new(combine.outputs[0],emit.inputs['Color'])
        l.new(emit.outputs[0],out.inputs['Surface'])
textures={}
for channel in ['base','orm','normal']:
    source_channel(channel)
    image=bpy.data.images.new(name+'-'+channel,width=texsize,height=texsize,alpha=False)
    if channel!='base':image.colorspace_settings.name='Non-Color'
    tex=nodes.new('ShaderNodeTexImage');tex.image=image;nodes.active=tex
    high.select_set(True);low.select_set(True);bpy.context.view_layer.objects.active=low
    bpy.ops.object.bake(type='NORMAL' if channel=='normal' else 'EMIT')
    image.pack();textures[channel]=tex
links.new(textures['base'].outputs['Color'],bsdf.inputs['Base Color'])
sep=nodes.new('ShaderNodeSeparateRGB');links.new(textures['orm'].outputs['Color'],sep.inputs[0])
links.new(sep.outputs[1],bsdf.inputs['Roughness']);links.new(sep.outputs[2],bsdf.inputs['Metallic'])
normal=nodes.new('ShaderNodeNormalMap');links.new(textures['normal'].outputs['Color'],normal.inputs['Color'])
links.new(normal.outputs[0],bsdf.inputs['Normal'])
# Exact contract bounds after simplification, with a bottom-centre origin.
lo,hi=bounds(low);size=hi-lo;origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for v in low.data.vertices:v.co=Vector(tuple((v.co[i]-origin[i])*target[i]/size[i] for i in range(3)))
low.data.update();low.data.calc_loop_triangles();triangles=len(low.data.loop_triangles)
high.select_set(False);low.select_set(True)
output=dest/(name+'.glb')
bpy.ops.export_scene.gltf(filepath=str(output),export_format='GLB',use_selection=True,
    export_yup=True,export_animations=False,export_cameras=False,export_lights=False)
raw=output.read_bytes();doc=json.loads(raw[20:20+struct.unpack_from('<I',raw,12)[0]])
triangles=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
record=dict(asset=name,provider='Tripo3D',task_id=provider['task_id'],
    source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),reference_sha256=provider['image_sha256'],
    sha256=hashlib.sha256(output.read_bytes()).hexdigest(),bytes=output.stat().st_size,
    source_triangles=source_triangles,triangles=triangles,dimensions=spec['dimensions'],texture_size=texsize,
    preparation='scripts/prepare-track-kit.py; welded/decimated, fresh UVs, source base/ORM/normal rebake, metric fit',
    coordinates='+Y up, bottom footprint centre; length +X, facade +Z',
    license_status=provider['license_status'])
(dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record),flush=True)
