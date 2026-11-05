"""Offline normalization of image-first Tripo track surface assets.

Blender --background --factory-startup --python this.py -- --source-root PATH
"""
import argparse
import hashlib
import importlib.util
import json
import math
import shutil
import sys
from pathlib import Path

import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'assets/runtime/models/dockside-track-surfaces'
QA=ROOT/'artifacts/dockside-track-surfaces'
spec=importlib.util.spec_from_file_location('warehouse_tools',ROOT/'scripts/prepare-dockside-warehouses.py')
H=importlib.util.module_from_spec(spec)
spec.loader.exec_module(H)
ASSETS={
    'track-straight':{'size':[12,16,.25],'budget':3000},
    'track-corner':{'size':[16,16,.25],'budget':5000},
    'track-curb':{'size':[4,.6,.15],'budget':1800},
    'drainage-grate':{'size':[2,.5,.12],'budget':6000},
}

def road_mesh(name):
    """Exact flat top and joining ends; surface detail comes from the Tripo bake."""
    if name=='track-straight':
        polygon=[(-6,-8),(6,-8),(6,8),(-6,8)]
        n=len(polygon)
        vertices=[(x,y,z) for z in [0,.25] for x,y in polygon]
        faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
        faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    else:
        segments=64
        bands=4
        vertices=[(r*math.cos(a*math.pi/2/segments)-8,r*math.sin(a*math.pi/2/segments)-8,z)
                  for z in [0,.25] for r in np.linspace(4,16,bands+1) for a in range(segments+1)]
        n=(bands+1)*(segments+1)
        faces=[]
        for band in range(bands):
            for a in range(segments):
                i=band*(segments+1)+a
                quad=(i,i+segments+1,i+segments+2,i+1)
                faces.append(tuple(reversed(quad)))
                faces.append(tuple(v+n for v in quad))
        perimeter=list(range(segments+1))
        perimeter += [b*(segments+1)+segments for b in range(1,bands+1)]
        perimeter += [bands*(segments+1)+a for a in reversed(range(segments))]
        perimeter += [b*(segments+1) for b in reversed(range(1,bands))]
        for i,j in zip(perimeter,perimeter[1:]+perimeter[:1]):faces.append((j,i,i+n,j+n))
    mesh=bpy.data.meshes.new(name+' exact slab')
    mesh.from_pydata(vertices,[],faces)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj

def share_road_material(obj):
    """Bend the verified straight's UVs around the quarter annulus."""
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(DEST/'track-straight.glb'))
    template=next(o for o in bpy.context.scene.objects if o not in before and o.type=='MESH')
    top=[p for p in template.data.polygons if (template.matrix_world.to_3x3()@p.normal).z>.9]
    positions=[]
    uvs=[]
    for polygon in top:
        for index in polygon.loop_indices:
            point=template.matrix_world@template.data.vertices[template.data.loops[index].vertex_index].co
            positions.append([point.x,point.y,1])
            uvs.append(template.data.uv_layers.active.data[index].uv[:])
    transform=np.linalg.lstsq(np.array(positions),np.array(uvs),rcond=None)[0]
    obj.data.materials.clear()
    obj.data.materials.append(template.data.materials[top[0].material_index])
    underside=next(p for p in template.data.polygons if (template.matrix_world.to_3x3()@p.normal).z<-.9)
    obj.data.materials.append(template.data.materials[underside.material_index])
    uv=obj.data.uv_layers.new(name='Curved road UV')
    for polygon in obj.data.polygons:
        polygon.material_index=0 if polygon.normal.z>.9 else 1
        for index in polygon.loop_indices:
            point=obj.data.vertices[obj.data.loops[index].vertex_index].co
            radius=math.hypot(point.x+8,point.y+8)
            theta=math.atan2(max(point.y+8,0),max(point.x+8,0))
            uv.data[index].uv=np.array([radius-10,theta*16/(math.pi/2)-8,1])@transform
    bpy.data.objects.remove(template,do_unlink=True)

def road_material(obj):
    material=obj.data.materials[0]
    nodes=material.node_tree.nodes
    # Damp asphalt is a rough dielectric; prevent provider metallic highlights
    # from reading as bumps on the flat driving surface.
    for node in nodes:
        if node.type=='NORMAL_MAP':node.inputs['Strength'].default_value=.25
        if node.type=='TEX_IMAGE' and node.image and node.image.name.endswith('-orm'):
            pixels=np.empty(len(node.image.pixels),dtype=np.float32)
            node.image.pixels.foreach_get(pixels)
            pixels=pixels.reshape((-1,4))
            pixels[:,1]=np.clip(pixels[:,1],.55,.9)
            pixels[:,2]=0
            node.image.pixels.foreach_set(pixels.ravel())
            node.image.update()
            node.image.pack()
    # Side/underside geometry is authored after normalization, so give it a
    # simple substrate material instead of projecting an unrelated top atlas.
    substrate=bpy.data.materials.new('Dark concrete road substrate')
    substrate.use_nodes=True
    shader=substrate.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value=(.035,.04,.045,1)
    shader.inputs['Roughness'].default_value=.85
    obj.data.materials.append(substrate)
    for polygon in obj.data.polygons:
        if polygon.normal.z<.5:polygon.material_index=1

def prepare(source_root,inspect=False,only=None):
    records=[]
    for name,settings in ASSETS.items():
        if only and name not in only:continue
        source=source_root/'artifacts/model-jobs'/('dockside-'+name)/'model.glb'
        if inspect and not source.exists():continue
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(source))
        objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
        bpy.ops.object.select_all(action='DESELECT')
        for o in objects:o.select_set(True)
        bpy.context.view_layer.objects.active=objects[0]
        if len(objects)>1:bpy.ops.object.join()
        obj=bpy.context.object
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        obj.name=name
        count=H.triangles(obj)
        assert count<=20000
        lo,hi=H.bounds([obj])
        original_size=list(hi-lo)
        correction={'method':'Rebuilt flat quarter annulus with shared straight-road PBR after provider geometry/UV distortion',
            'inner_radius_m':4,'outer_radius_m':16,'angle_degrees':90} if name=='track-corner' else None
        lo,hi=H.bounds([obj])
        rotation=0
        rotate=(name=='track-straight' and (hi-lo).x>(hi-lo).y) or (name in {'track-curb','drainage-grate'} and (hi-lo).x<(hi-lo).y)
        if rotate:
            rotation=90
            for v in obj.data.vertices:v.co=Matrix.Rotation(math.pi/2,4,'Z')@v.co
        H.fit(obj,settings['size'])
        maps=[]
        for im in list(bpy.data.images):
            if im.type!='IMAGE':continue
            maps.append({'name':im.name,'size':list(im.size)})
            if max(im.size)>2048:im.scale(2048,2048)
            im.pack()
        if inspect:
            H.render([obj],QA/(name+'-source.png'),(-1,-1,1.8),size=(1200,1000))
            H.render([obj],QA/(name+'-source-top.png'),(0,-.001,3),size=(1000,1000))
            print('SOURCE '+json.dumps({'id':name,'triangles':count,'size':original_size,'rotation':rotation}),flush=True)
            continue
        source_obj=obj
        if name in {'track-straight','track-corner'}:
            if name=='track-straight':
                for vertex in source_obj.data.vertices:vertex.co.z=.25 if vertex.co.z>.125 else 0
            obj=road_mesh(name)
        else:
            obj=bpy.data.objects.new(name,source_obj.data.copy())
            bpy.context.scene.collection.objects.link(obj)
            H.reduce(obj,settings['budget'])
            H.fit(obj,settings['size'])
        if name=='track-corner':
            share_road_material(obj)
        else:
            H.bake_material(source_obj,obj,name)
            if name=='track-straight':road_material(obj)
        source_obj.hide_render=True
        source_obj.hide_set(True)
        obj['asset_id']='dockside-'+name
        obj['units']='meters'
        output_name='track-corner-left' if name=='track-corner' else name
        obj.name=output_name
        result=H.export(DEST/(output_name+'.glb'),[obj])
        H.render([obj],QA/(output_name+'-front.png'),(-1,-1,1.8),size=(1200,1000))
        H.render([obj],QA/(output_name+'-back.png'),(1,1,1.8),size=(1200,1000))
        bpy.ops.wm.save_as_mainfile(filepath=str(QA/(output_name+'.blend')),compress=True)
        provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
        record={'id':output_name,'source_id':name,**result,'dimensions_xyz_gltf':[settings['size'][0],settings['size'][2],settings['size'][1]],
            'units':'meters','up_axis':'Y','pivot':'bottom center','source_triangles':count,
            'source_dimensions_blender':original_size,'source_rotation_z_degrees':rotation,
            'original_maps':maps,'texture_cap':1024,'pbr_rebaked':name!='track-corner',
            'material_source_asset':'track-straight' if name=='track-corner' else name,
            'provider':'Tripo3D','task_id':provider['task_id'],'credits_consumed':provider.get('credits_consumed'),
            'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'reference_sha256':provider['image_sha256'],'license_status':provider['license_status'],
            'request':{k:v for k,v in provider['request'].items() if k!='input'}}
        record['geometry_correction']=correction
        record['flat_road_top_m']=.25 if name in {'track-straight','track-corner'} else None
        if name=='track-straight':record['join_centers_xyz_gltf']=[[0,.25,8],[0,.25,-8]]
        if name=='track-corner':record['join_centers_xyz_gltf']=[[2,.25,8],[-8,.25,-2]]
        H.save_json(DEST/(output_name+'.asset.json'),record)
        shutil.copyfile(source_root/'artifacts/model-input-images'/('dockside-'+name)/'reference.json',DEST/(output_name+'.reference.json'))
        records.append(record)
        print('PREPARED '+json.dumps(record),flush=True)
        if name=='track-corner':
            for vertex in obj.data.vertices:vertex.co.x=-vertex.co.x
            bm=bmesh.new()
            bm.from_mesh(obj.data)
            bmesh.ops.reverse_faces(bm,faces=list(bm.faces))
            bm.to_mesh(obj.data)
            bm.free()
            obj.name='track-corner-right'
            obj['asset_id']='dockside-track-corner-right'
            result=H.export(DEST/'track-corner-right.glb',[obj])
            H.render([obj],QA/'track-corner-right-front.png',(-1,-1,1.8),size=(1200,1000))
            H.render([obj],QA/'track-corner-right-back.png',(1,1,1.8),size=(1200,1000))
            bpy.ops.wm.save_as_mainfile(filepath=str(QA/'track-corner-right.blend'),compress=True)
            mirror={**record,**result,'id':'track-corner-right','derived_from':'track-corner-left',
                'credits_consumed':0,'join_centers_xyz_gltf':[[-2,.25,8],[8,.25,-2]],
                'derivation':'Offline X reflection with corrected face winding; shared source task and maps'}
            H.save_json(DEST/'track-corner-right.asset.json',mirror)
            shutil.copyfile(DEST/'track-corner-left.reference.json',DEST/'track-corner-right.reference.json')
    if not inspect:
        records=[json.loads(p.read_text()) for p in sorted(DEST.glob('*.asset.json'))]
        H.save_json(DEST/'manifest.json',{'assets':records,'credits_consumed':sum(r['credits_consumed'] for r in records),
            'scope':'Static surface assets; no race logic or engine integration'})

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,required=True)
    parser.add_argument('--inspect',action='store_true')
    parser.add_argument('--only',nargs='+',choices=list(ASSETS))
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    DEST.mkdir(parents=True,exist_ok=True)
    QA.mkdir(parents=True,exist_ok=True)
    prepare(args.source_root,args.inspect,args.only)
