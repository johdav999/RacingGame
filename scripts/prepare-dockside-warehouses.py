"""Prepare six Tripo modules and assemble four grid-based warehouse exteriors.

Run with Blender 3.5+: --background --factory-startup --python this.py --
    --source-root PATH [--modules-only] [--assemble-only]
No provider calls are made by this script.
"""
import argparse
import hashlib
import json
import math
import shutil
import struct
import sys
from collections import Counter
from pathlib import Path

import bpy
import bmesh
from mathutils import Matrix, Vector

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'assets/runtime/models/dockside-warehouses'
QA=ROOT/'artifacts/dockside-warehouses'
GRID=4.0
FOUNDATION=.20
# Dimensions are Blender X width, Y depth, Z height, in meters.
MODULES={
    'wall': {'size':(4,.30,4),'detail_budget':1800,'assembly_budget':420},
    'corner': {'size':(.42,.42,4),'detail_budget':500,'assembly_budget':120},
    'roof': {'size':(4,4,.22),'detail_budget':900,'assembly_budget':180},
    'door': {'size':(3.5,.22,3.6),'detail_budget':2000,'assembly_budget':650},
    'vent': {'size':(1.2,1.2,1),'detail_budget':1200,'assembly_budget':760},
    'window': {'size':(2.8,.18,1.2),'detail_budget':1200,'assembly_budget':240},
}
BUILDINGS=[
    {'id':'compact-depot','label':'Compact dock depot','nx':3,'ny':3,'height':4,
     'cells':[(x,y) for x in range(3) for y in range(3)],
     'doors':[(0,0,'S'),(1,0,'S')],'vents':[(1,1),(2,2)],'windows':'sparse'},
    {'id':'long-freight-shed','label':'Long freight shed','nx':6,'ny':3,'height':4,
     'cells':[(x,y) for x in range(6) for y in range(3)],
     'doors':[(x,0,'S') for x in range(1,6)],'vents':[(1,1),(4,1)],'windows':'sparse',
     'skip_windows':[(0,0,'S')]},
    {'id':'high-bay-store','label':'High-bay warehouse','nx':4,'ny':4,'height':8,
     'cells':[(x,y) for x in range(4) for y in range(4)],
     'doors':[(0,0,'S'),(1,0,'S'),(3,1,'E')],'vents':[(1,1),(2,2)],'windows':'high'},
    {'id':'l-shaped-transfer','label':'L-shaped transfer warehouse','nx':5,'ny':5,'height':4,
     'cells':[(x,y) for x in range(5) for y in range(5) if x<2 or y<2],
     'doors':[(0,0,'S'),(1,0,'S'),(3,1,'N')],'vents':[(1,3),(4,0)],'windows':'sparse'},
]


def save_json(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps(data,indent=2)+'\n',encoding='utf8')


def triangles(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def bounds(objects,world=False):
    coords=[o.matrix_world@v.co if world else v.co for o in objects for v in o.data.vertices]
    return (Vector([min(v[i] for v in coords) for i in range(3)]),
            Vector([max(v[i] for v in coords) for i in range(3)]))


def fit(obj,size):
    lo,hi=bounds([obj])
    extent=hi-lo
    assert min(extent)>0
    origin=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    for v in obj.data.vertices:
        v.co=Vector([(v.co[i]-origin[i])*size[i]/extent[i] for i in range(3)])
    obj.data.update()
    # Source split normals no longer describe rotated/rescaled/decimated vertices.
    bpy.context.view_layer.objects.active=obj
    if obj.data.has_custom_normals:
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    for polygon in obj.data.polygons:polygon.use_smooth=True
    obj.data.use_auto_smooth=True
    obj.data.auto_smooth_angle=math.radians(45)
    assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)


def reduce(obj,limit):
    bpy.context.view_layer.objects.active=obj
    # Tripo/glTF duplicates geometric vertices at UV/normal seams. Collapse
    # decimation on disconnected triangles punches holes unless we weld first.
    mesh=bmesh.new()
    mesh.from_mesh(obj.data)
    bmesh.ops.remove_doubles(mesh,verts=list(mesh.verts),dist=.0001)
    bmesh.ops.recalc_face_normals(mesh,faces=list(mesh.faces))
    mesh.to_mesh(obj.data)
    mesh.free()
    obj.data.update()
    count=triangles(obj)
    if count>limit:
        mod=obj.modifiers.new('Reusable prop budget','DECIMATE')
        mod.ratio=(limit-4)/count
        mod.use_collapse_triangulate=True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    assert 0<triangles(obj)<=limit,(obj.name,triangles(obj),limit)


def export(path,objects):
    path.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,
        export_yup=True,export_animations=False,export_cameras=False,export_lights=False,
        export_extras=True)
    raw=path.read_bytes()
    doc=json.loads(raw[20:20+struct.unpack_from('<I',raw,12)[0]])
    mesh_tris=[sum(doc['accessors'][p['indices']]['count']//3 for p in m['primitives']) for m in doc['meshes']]
    instanced=sum(mesh_tris[n['mesh']] for n in doc['nodes'] if 'mesh' in n)
    return {'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),
            'triangles_instanced':instanced,'triangles_unique':sum(mesh_tris),
            'mesh_instances':sum('mesh' in n for n in doc['nodes']),
            'unique_meshes':len(mesh_tris),'materials':len(doc['materials'])}


def bake_material(source,target,label):
    """Reproject source PBR onto fresh UVs after simplification.

    Collapsing geometry across Tripo's many UV islands without rebaking stretches
    the atlas. Baking from the original model preserves its authored appearance.
    """
    scene=bpy.context.scene
    scene.render.engine='CYCLES'
    scene.cycles.device='CPU'
    scene.cycles.samples=1
    scene.render.bake.use_selected_to_active=True
    scene.render.bake.cage_extrusion=.12
    scene.render.bake.max_ray_distance=.50
    scene.render.bake.margin=8
    bpy.ops.object.select_all(action='DESELECT')
    target.select_set(True)
    bpy.context.view_layer.objects.active=target
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.025)
    bpy.ops.object.mode_set(mode='OBJECT')
    source.select_set(True)
    material=bpy.data.materials.new('Warehouse_'+label)
    material.use_nodes=True
    target.data.materials.clear()
    target.data.materials.append(material)
    nodes,links=material.node_tree.nodes,material.node_tree.links
    bsdf=nodes.get('Principled BSDF')
    maps={}
    for kind in ['albedo','normal','orm']:
        image=bpy.data.images.new(label+'-'+kind,width=1024,height=1024,alpha=False)
        image.colorspace_settings.name='sRGB' if kind=='albedo' else 'Non-Color'
        node=nodes.new('ShaderNodeTexImage')
        node.image=image
        nodes.active=node
        if kind=='albedo':
            bpy.ops.object.bake(type='DIFFUSE',pass_filter={'COLOR'})
        elif kind=='normal':
            bpy.ops.object.bake(type='NORMAL')
        else:
            restores=[]
            for mat in source.data.materials:
                tree=mat.node_tree
                source_bsdf=next(n for n in tree.nodes if n.type=='BSDF_PRINCIPLED')
                orm=source_bsdf.inputs['Metallic'].links[0].from_node.inputs[0].links[0].from_node
                output=next(n for n in tree.nodes if n.type=='OUTPUT_MATERIAL')
                old=output.inputs['Surface'].links[0].from_socket
                emission=tree.nodes.new('ShaderNodeEmission')
                tree.links.new(orm.outputs['Color'],emission.inputs['Color'])
                tree.links.new(emission.outputs[0],output.inputs['Surface'])
                restores.append((tree,output,old,emission))
            bpy.ops.object.bake(type='EMIT')
            for tree,output,old,emission in restores:
                tree.links.new(old,output.inputs['Surface'])
                tree.nodes.remove(emission)
        image.pack()
        maps[kind]=node
    links.new(maps['albedo'].outputs['Color'],bsdf.inputs['Base Color'])
    normal=nodes.new('ShaderNodeNormalMap')
    links.new(maps['normal'].outputs['Color'],normal.inputs['Color'])
    links.new(normal.outputs['Normal'],bsdf.inputs['Normal'])
    separate=nodes.new('ShaderNodeSeparateRGB')
    links.new(maps['orm'].outputs['Color'],separate.inputs[0])
    links.new(separate.outputs['G'],bsdf.inputs['Roughness'])
    links.new(separate.outputs['B'],bsdf.inputs['Metallic'])
    scene.render.bake.use_selected_to_active=False


def render(objects,path,direction=(-1.5,-2,1.25),size=(1500,1000)):
    # Only light/camera objects are replaced. Geometry stays exactly as exported.
    for ob in list(bpy.context.scene.objects):
        if ob.type in {'LIGHT','CAMERA'}:bpy.data.objects.remove(ob,do_unlink=True)
    bpy.context.view_layer.update()
    lo,hi=bounds(objects,True)
    center,extent=(lo+hi)/2,max(hi-lo)
    scene=bpy.context.scene
    scene.render.engine='BLENDER_EEVEE'
    scene.eevee.use_gtao=True
    scene.eevee.gtao_distance=.8
    scene.eevee.taa_render_samples=48
    scene.render.resolution_x,scene.render.resolution_y=size
    scene.render.resolution_percentage=100
    if not scene.world:scene.world=bpy.data.worlds.new('Warehouse studio')
    scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.18,.20,.24,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.8
    scene.view_settings.view_transform='Standard'
    scene.view_settings.look='Medium High Contrast'
    for name,offset,power in [('Key',(-1,-2,3),100),('Fill',(2,-1,1.5),65),('Rim',(1,2,3),90)]:
        light=bpy.data.lights.new(name,'AREA')
        light.energy=power*extent*extent
        light.size=extent*1.5
        ob=bpy.data.objects.new(name,light)
        scene.collection.objects.link(ob)
        ob.location=center+Vector(offset)*extent
        ob.rotation_euler=(center-ob.location).to_track_quat('-Z','Y').to_euler()
    cam=bpy.data.objects.new('Review',bpy.data.cameras.new('Review'))
    scene.collection.objects.link(cam)
    scene.camera=cam
    cam.data.type='ORTHO'
    cam.location=center+Vector(direction)*extent
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    rotation=cam.rotation_euler.to_matrix().transposed()
    projected=[rotation@(o.matrix_world@v.co-center) for o in objects for v in o.data.vertices]
    width=max(v.x for v in projected)-min(v.x for v in projected)
    height=max(v.y for v in projected)-min(v.y for v in projected)
    cam.data.ortho_scale=max(width,height*size[0]/size[1])*1.15
    scene.render.filepath=str(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.render.render(write_still=True)


def prepare_modules(source_root,start_at=None):
    records=[]
    for name,settings in MODULES.items():
        if start_at and list(MODULES).index(name)<list(MODULES).index(start_at):
            record=json.loads((DEST/'modules'/(name+'.asset.json')).read_text())
            assert record['assembly_lod'] and record['assembly_budget']==settings['assembly_budget']
            records.append(record)
            continue
        source=source_root/'artifacts/model-jobs'/('warehouse-'+name)/'model.glb'
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(source))
        meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        bpy.ops.object.select_all(action='DESELECT')
        for o in meshes:o.select_set(True)
        bpy.context.view_layer.objects.active=meshes[0]
        if len(meshes)>1:bpy.ops.object.join()
        obj=bpy.context.object
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        obj.name=name
        source_count=triangles(obj)
        assert source_count<=20000
        lo,hi=bounds([obj])
        source_size=list(hi-lo)
        if name in {'wall','door','window'} and (hi-lo).x<(hi-lo).y:
            for v in obj.data.vertices:v.co=Matrix.Rotation(math.pi/2,4,'Z')@v.co
        fit(obj,settings['size'])
        original_maps=[]
        for im in list(bpy.data.images):
            if im.type!='IMAGE':continue
            original_maps.append({'name':im.name,'size':list(im.size)})
            w,h=im.size
            if max(w,h)>1024:im.scale(round(w*1024/max(w,h)),round(h*1024/max(w,h)))
            im.pack()
        source_obj=obj
        variants={}
        for variant,budget in [('detail',settings['detail_budget']),('assembly',settings['assembly_budget'])]:
            obj=bpy.data.objects.new(name+'-'+variant,source_obj.data.copy())
            bpy.context.scene.collection.objects.link(obj)
            reduce(obj,budget)
            fit(obj,settings['size'])
            source_obj.hide_render=False
            source_obj.hide_set(False)
            bake_material(source_obj,obj,name+'-'+variant)
            source_obj.hide_render=True
            source_obj.hide_set(True)
            obj['module']=name
            obj['grid_m']=GRID
            obj['dimensions_xyz_blender']=list(settings['size'])
            filename=name if variant=='detail' else name+'-assembly'
            variants[variant]=export(DEST/'modules'/(filename+'.glb'),[obj])
            render([obj],QA/'modules'/(filename+'-front.png'),(-1,-2,1.0))
            render([obj],QA/'modules'/(filename+'-back.png'),(1,2,1.0))
            if variant=='detail':
                bpy.ops.wm.save_as_mainfile(filepath=str(QA/'modules'/(name+'.blend')),compress=True)
            bpy.data.objects.remove(obj,do_unlink=True)
        result=variants['detail']
        provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
        record={'name':name,**settings,**result,'assembly_lod':variants['assembly'],'source_triangles':source_count,
                'source_dimensions_blender':source_size,'original_maps':original_maps,'texture_cap':1024,
                'provider':'Tripo3D','task_id':provider['task_id'],
                'credits_consumed':provider.get('credits_consumed'),
                'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                'reference_sha256':provider['image_sha256'],'license_status':provider['license_status']}
        save_json(DEST/'modules'/(name+'.asset.json'),record)
        shutil.copyfile(source_root/'artifacts/model-input-images'/('warehouse-'+name)/'reference.json',
                        DEST/'modules'/(name+'.reference.json'))
        records.append(record)
        print('MODULE '+json.dumps(record),flush=True)
    save_json(DEST/'modules/manifest.json',{'modules':records,'units':'meters','up_axis':'Y',
                'front_axis':'+Z','grid_m':GRID,'credits_consumed':sum(r['credits_consumed'] for r in records)})


def make_cube_mesh(name,size):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj=bpy.context.object
    obj.name=name
    obj.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for v in obj.data.vertices:v.co.z+=size[2]/2
    mat=bpy.data.materials.get('Assembly support') or bpy.data.materials.new('Assembly support')
    mat.use_nodes=True
    bsdf=mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value=(.14,.15,.16,1)
    bsdf.inputs['Roughness'].default_value=.85
    obj.data.materials.append(mat)
    return obj


def boundary(cells):
    cells=set(map(tuple,cells))
    edges=[]
    for x,y in sorted(cells):
        for dx,dy,side,location,angle,ends in [
            (0,-1,'S',(4*x+2,4*y),0,((x,y),(x+1,y))),
            (1,0,'E',(4*x+4,4*y+2),90,((x+1,y),(x+1,y+1))),
            (0,1,'N',(4*x+2,4*y+4),180,((x,y+1),(x+1,y+1))),
            (-1,0,'W',(4*x,4*y+2),-90,((x,y),(x,y+1))),
        ]:
            if (x+dx,y+dy) not in cells:
                edges.append({'cell':(x,y),'side':side,'xy':location,'angle':angle,'ends':ends})
    return edges


def assemble_buildings():
    reports=[]
    for spec in BUILDINGS:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        templates={}
        for name,settings in MODULES.items():
            before=set(bpy.context.scene.objects)
            bpy.ops.import_scene.gltf(filepath=str(DEST/'modules'/(name+'-assembly.glb')))
            obj=next(o for o in bpy.context.scene.objects if o not in before and o.type=='MESH')
            templates[name]=obj
        templates['foundation']=make_cube_mesh('foundation',(4,4,FOUNDATION))
        templates['roof-backing']=make_cube_mesh('roof-backing',(4,4,.06))
        # Closed exterior assets need planar inner sheets behind approximate AI edges.
        templates['wall-backing']=make_cube_mesh('wall-backing',(4,.08,4))
        for obj in templates.values():obj.hide_render=True
        instances=[]
        placements=[]
        offset=Vector((spec['nx']*2,spec['ny']*2,0))
        def add(kind,xyz,angle=0,scale=(1,1,1)):
            template=templates[kind]
            obj=bpy.data.objects.new(kind+'_%03d'%len(instances),template.data)
            bpy.context.scene.collection.objects.link(obj)
            obj.location=Vector(xyz)-offset
            obj.rotation_euler.z=math.radians(angle)
            obj.scale=scale
            obj['module']=kind
            instances.append(obj)
            placements.append({'name':obj.name,'module':kind,
                'translation':[obj.location.x,obj.location.z,-obj.location.y],
                'rotation_y_degrees':angle,'scale':[scale[0],scale[2],scale[1]]})
            return obj
        height=spec['height']
        edges=boundary(spec['cells'])
        doors=set(spec['doors'])
        assert all(any((*e['cell'],e['side'])==d for e in edges) for d in doors)
        vertices=Counter(v for edge in edges for v in edge['ends'])
        assert all(count==2 for count in vertices.values()),'Non-manifold footprint boundary'
        for x,y in sorted(spec['cells']):
            add('foundation',(4*x+2,4*y+2,0))
            add('roof',(4*x+2,4*y+2,FOUNDATION+height))
            add('roof-backing',(4*x+2,4*y+2,FOUNDATION+height-.05))
        for idx,edge in enumerate(edges):
            x,y=edge['xy']
            angle=edge['angle']
            theta=math.radians(angle)
            outward=Vector((math.sin(theta),-math.cos(theta),0))
            add('wall',(x,y,FOUNDATION),angle,(1,1,height/4))
            inner=Vector((x,y,FOUNDATION))-outward*.10
            add('wall-backing',inner,angle,(1,1,height/4))
            attach=Vector((x,y,FOUNDATION))+outward*.24
            key=(*edge['cell'],edge['side'])
            if key in doors:
                add('door',attach,angle,(1,1,1.5 if height>4 else 1))
            elif (idx%2==0 or spec['windows']=='high') and key not in spec.get('skip_windows',[]):
                attach.z+=height-1.65
                add('window',attach,angle)
        for x,y in sorted(vertices):
            add('corner',(x*4,y*4,FOUNDATION),0,(1,1,height/4))
        for x,y in spec['vents']:
            assert (x,y) in spec['cells']
            add('vent',(4*x+2,4*y+2,FOUNDATION+height+.21))
        for obj in templates.values():bpy.data.objects.remove(obj,do_unlink=True)
        bpy.context.view_layer.update()
        count=sum(triangles(o) for o in instances)
        assert count<=20000,(spec['id'],count)
        lo,hi=bounds(instances,True)
        assert abs(lo.z)<1e-5
        output=DEST/'warehouses'/(spec['id']+'.glb')
        stats=export(output,instances)
        assert stats['triangles_instanced']==count
        record={'id':spec['id'],'label':spec['label'],'grid_m':GRID,
            'footprint_cells':spec['cells'],'nominal_footprint_m':[spec['nx']*4,spec['ny']*4],
            'wall_height_m':height,'dimensions_m_xyz_gltf':[hi.x-lo.x,hi.z-lo.z,hi.y-lo.y],
            'bounds_min_xyz_gltf':[lo.x,lo.z,-hi.y],'bounds_max_xyz_gltf':[hi.x,hi.z,-lo.y],
            'module_counts':dict(Counter(p['module'] for p in placements)),**stats,
            'components':placements,'doors_closed':True,'interiors':False}
        save_json(DEST/'warehouses'/(spec['id']+'.assembly.json'),record)
        render(instances,QA/'warehouses'/(spec['id']+'-front.png'))
        render(instances,QA/'warehouses'/(spec['id']+'-back.png'),(1.5,2,1.3))
        bpy.ops.wm.save_as_mainfile(filepath=str(QA/'warehouses'/(spec['id']+'.blend')),compress=True)
        reports.append({k:v for k,v in record.items() if k!='components'})
        print('ASSEMBLY '+json.dumps(reports[-1]),flush=True)
    save_json(DEST/'manifest.json',{'warehouses':reports,'module_manifest':'modules/manifest.json',
        'scope':'Modular exterior assets; no scene placement or game runtime changes',
        'authored_supports':['foundation','roof-backing','wall-backing']})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=ROOT)
    parser.add_argument('--start-at',choices=list(MODULES),help='Resume offline module preparation at this component')
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--modules-only',action='store_true')
    mode.add_argument('--assemble-only',action='store_true')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    DEST.mkdir(parents=True,exist_ok=True)
    QA.mkdir(parents=True,exist_ok=True)
    if not args.assemble_only:prepare_modules(args.source_root.resolve(),args.start_at)
    if not args.modules_only:assemble_buildings()


if __name__=='__main__':main()
