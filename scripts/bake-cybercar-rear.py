"""Blender-only bevel/normal/UV and procedural surface bake for authored rear.

Keeps source coordinates (+Y up). Output is plain geometry data and two shared
1024px maps; refine-cybercar.py inserts them without re-exporting original parts.
"""
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path

work=Path(sys.argv[sys.argv.index('--')+1])
spec=json.loads((work/'source.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=16
scene.cycles.device='CPU';scene.render.bake.margin=8
scene.view_settings.view_transform='Standard'

# Each loft is a separate closed construction piece. Do not weld intersecting
# louver/wing panels into each other; open shoulder sheets get a thin backing.
objects=[]
for index,piece in enumerate(spec['pieces']):
    mesh=bpy.data.meshes.new('Rear panel')
    mesh.from_pydata(piece['vertices'],[],piece['polygons']);mesh.update()
    obj=bpy.data.objects.new('Panel %03d'%index,mesh);scene.collection.objects.link(obj)
    bpy.context.view_layer.objects.active=obj;obj.select_set(True)
    bm=bmesh.new();bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    bmesh.ops.dissolve_limit(bm,angle_limit=.01,verts=list(bm.verts),edges=list(bm.edges))
    opened=any(e.is_boundary for e in bm.edges)
    bm.to_mesh(mesh);bm.free()
    if opened:
        solid=obj.modifiers.new('Shoulder panel thickness','SOLIDIFY');solid.thickness=.0015;solid.offset=-1
        bpy.ops.object.modifier_apply(modifier=solid.name)
    bevel=obj.modifiers.new('Small manufactured edge radius','BEVEL')
    bevel.width=.0012;bevel.segments=2;bevel.limit_method='ANGLE';bevel.angle_limit=.30
    bevel.use_clamp_overlap=True;bevel.harden_normals=True
    mesh.use_auto_smooth=True;mesh.auto_smooth_angle=math.pi
    for polygon in mesh.polygons:polygon.use_smooth=True
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    weighted=obj.modifiers.new('Area weighted panel normals','WEIGHTED_NORMAL')
    weighted.keep_sharp=True;weighted.weight=50;weighted.mode='FACE_AREA_WITH_ANGLE'
    bpy.ops.object.modifier_apply(modifier=weighted.name)
    obj['material']=piece['material'];objects.append(obj);obj.select_set(False)

# Join by material to preserve the five established runtime component names.
groups={}
by_material={key:[o for o in objects if o['material']==int(key)] for key in spec['materials']}
for key in spec['materials']:
    selected=by_material[key]
    for obj in selected:obj.select_set(True)
    bpy.context.view_layer.objects.active=selected[0]
    if len(selected)>1:bpy.ops.object.join()
    obj=bpy.context.object;obj.name='rear_'+spec['families'][key]
    groups[key]=obj;obj.select_set(False)
for obj in groups.values():obj.select_set(True)
bpy.context.view_layer.objects.active=next(iter(groups.values()))
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012,area_weight=.4,scale_to_bounds=True)
bpy.ops.object.mode_set(mode='OBJECT')

def math_node(nodes,links,op,a,b=None):
    n=nodes.new('ShaderNodeMath');n.operation=op
    for index,value in enumerate([a,b]):
        if value is None:continue
        if isinstance(value,(int,float)):n.inputs[index].default_value=value
        else:links.new(value,n.inputs[index])
    return n.outputs[0]

# Bake broad manufactured seams in source space, so they follow the body rather
# than arbitrary triangulation. AO records louver gaps and attached vent depth.
materials=[]
for key,obj in groups.items():
    source=spec['materials'][key]['pbrMetallicRoughness'];family=spec['families'][key]
    mat=bpy.data.materials.new(family);mat.use_nodes=True;obj.data.materials.clear();obj.data.materials.append(mat)
    nodes=mat.node_tree.nodes;nodes.clear();links=mat.node_tree.links
    output=nodes.new('ShaderNodeOutputMaterial');emission=nodes.new('ShaderNodeEmission')
    links.new(emission.outputs[0],output.inputs['Surface'])
    geometry=nodes.new('ShaderNodeNewGeometry');xyz=nodes.new('ShaderNodeSeparateXYZ');links.new(geometry.outputs['Position'],xyz.inputs[0])
    x,y,z=[xyz.outputs[a] for a in 'XYZ']
    M=lambda op,a,b=None:math_node(nodes,links,op,a,b)
    noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=180;noise.inputs['Detail'].default_value=2
    links.new(geometry.outputs['Position'],noise.inputs['Vector'])
    variation=M('ADD',M('MULTIPLY',noise.outputs['Fac'],.08),.96)
    ao=nodes.new('ShaderNodeAmbientOcclusion');ao.inputs['Distance'].default_value=.025;ao.samples=16
    occlusion=M('ADD',M('MULTIPLY',ao.outputs['AO'],.28),.72)
    seam=0
    if family=='bodywork':
        # Shoulder service-panel joint, tail fascia joint, and rear-deck seam.
        shoulder=M('MULTIPLY',M('GREATER_THAN',M('ABSOLUTE',z),.182),M('GREATER_THAN',y,.18))
        line=M('LESS_THAN',M('ABSOLUTE',M('SUBTRACT',x,.377)),.0009)
        seam=M('MULTIPLY',shoulder,line)
        tail=M('MULTIPLY',M('GREATER_THAN',x,.481),M('LESS_THAN',M('ABSOLUTE',M('SUBTRACT',y,.132)),.0008))
        seam=M('MAXIMUM',seam,tail)
        roof=M('MULTIPLY',M('GREATER_THAN',y,.279),M('LESS_THAN',M('ABSOLUTE',M('SUBTRACT',x,.055)),.00065))
        rail=M('LESS_THAN',M('ABSOLUTE',M('SUBTRACT',M('ABSOLUTE',z),M('ADD',.177,M('MULTIPLY',M('SUBTRACT',x,.17),.15)))),.00065)
        rail=M('MULTIPLY',rail,M('MULTIPLY',M('GREATER_THAN',x,.20),M('LESS_THAN',x,.35)))
        rail=M('MULTIPLY',rail,M('GREATER_THAN',y,.183))
        seam=M('MAXIMUM',seam,M('MAXIMUM',roof,rail))
    detail=M('MULTIPLY',M('MULTIPLY',variation,occlusion),M('SUBTRACT',1,M('MULTIPLY',seam,.65)))
    color=nodes.new('ShaderNodeMixRGB');color.blend_type='MULTIPLY';color.inputs[0].default_value=1
    color.inputs[1].default_value=source['baseColorFactor'];links.new(detail,color.inputs[2])
    rough=M('ADD',source['roughnessFactor'],M('MULTIPLY',M('SUBTRACT',noise.outputs['Fac'],.5),.08 if family!='glass' else .015))
    rough=M('MINIMUM',M('ADD',rough,M('MULTIPLY',seam,.16)),1)
    mr=nodes.new('ShaderNodeCombineRGB');mr.inputs['R'].default_value=1
    links.new(rough,mr.inputs['G']);mr.inputs['B'].default_value=source['metallicFactor']
    image_node=nodes.new('ShaderNodeTexImage');nodes.active=image_node
    materials.append((mat,emission,image_node,color.outputs[0],mr.outputs[0]))

for semantic,slot in [('base-color',3),('metallic-roughness',4)]:
    image=bpy.data.images.new(semantic,1024,1024,alpha=False)
    image.colorspace_settings.name='sRGB' if semantic=='base-color' else 'Non-Color'
    for entry in materials:
        mat,emission,target=entry[:3];target.image=image;mat.node_tree.nodes.active=target
        mat.node_tree.links.new(entry[slot],emission.inputs['Color'])
    scene.render.bake.use_clear=True
    bpy.ops.object.bake(type='EMIT')
    image.filepath_raw=str(work/(semantic+'.png'));image.file_format='PNG';image.save()

result={}
for key,obj in groups.items():
    mesh=obj.data;mesh.calc_loop_triangles();mesh.calc_normals_split()
    p=[];n=[];uv=[];indices=[]
    for tri in mesh.loop_triangles:
        a,b,c=[mesh.vertices[index].co for index in tri.vertices]
        face=(b-a).cross(c-a)
        if face.length_squared<1e-18:continue
        face.normalize()
        for loop_id in tri.loops:
            loop=mesh.loops[loop_id]
            normal=loop.normal.copy()
            if normal.length_squared<.5 or normal.dot(face)<=0:normal=face.copy()
            normal.normalize()
            p.append(list(mesh.vertices[loop.vertex_index].co));n.append(list(normal))
            # Blender UV +V is up; GLB texture V is down.
            value=mesh.uv_layers.active.data[loop_id].uv;uv.append([value.x,1-value.y])
            indices.append(len(indices))
    result[key]=dict(positions=p,normals=n,uvs=uv,indices=indices)
result['evidence']=dict(bevel_source_units=.0012,bevel_world_mm=5.16,bevel_segments=2,
    normals='face-area-and-angle weighted split normals',atlas_size=[1024,1024],
    maps=['base-color','metallic-roughness'],baked_detail='source-space panel seams, louver/vent AO, subtle roughness variation',
    triangles=sum(len(v['indices'])//3 for k,v in result.items() if k!='evidence'))
(work/'prepared.json').write_text(json.dumps(result))
bpy.ops.wm.save_as_mainfile(filepath=str(work/'rear-authoring.blend'))
print(json.dumps(result['evidence']))
