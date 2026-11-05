"""Normalize and inspect the ten image-first Tripo harbor assets offline.

Blender 3.5+: --background --factory-startup --python this.py -- --source-root PATH
The original provider files are retained. No provider API calls are made.
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
from mathutils import Matrix, Vector

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'assets/runtime/models/dockside-harbor'
QA=ROOT/'artifacts/dockside-harbor'
spec=importlib.util.spec_from_file_location('warehouse_tools',ROOT/'scripts/prepare-dockside-warehouses.py')
H=importlib.util.module_from_spec(spec)
spec.loader.exec_module(H)
# Blender X width, Y depth, Z height. Uniform dimensions preserve complex props.
ASSETS={
    'quay-wall':{'size':[8,1,4],'budget':3000},
    'quay-corner':{'size':[4,4,4],'budget':4000},
    'quay-coping':{'size':[4,1.2,.25],'budget':1000},
    'dock-edge':{'size':[4,4,.22],'budget':3000},
    'concrete-barrier':{'size':[4,.7,1.1],'budget':4000},
    'guardrail':{'size':[4,.35,.85]},
    'chainlink-fence':{'size':[4,.3,2.4]},
    'floodlight-mast':{'height':12},
    'wall-lamp':{'width':.6},
    'container-crane':{'height':42},
}

def repair_fence(obj):
    """Keep the textured Tripo frame and replace its damaged interior wire web."""
    bm=bmesh.new()
    bm.from_mesh(obj.data)
    remove=[v for v in bm.verts if abs(v.co.x)<1.74 and .33<v.co.z<2.17]
    bmesh.ops.delete(bm,geom=remove,context='VERTS')
    bm.to_mesh(obj.data)
    bm.free()
    material=bpy.data.materials.new('Galvanized_chainlink_wire')
    material.use_nodes=True
    shader=material.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value=(.32,.37,.40,1)
    shader.inputs['Metallic'].default_value=.9
    shader.inputs['Roughness'].default_value=.42
    # Crossed round wires with a small front/back offset. No opacity overdraw.
    verts=[]
    faces=[]
    xmin,xmax,zmin,zmax=-1.79,1.79,.30,2.20
    for slope in [-1,1]:
        for k in range(-40,41):
            intercept=k*.16
            points=[]
            for x in [xmin,xmax]:
                z=slope*x+intercept
                if zmin<=z<=zmax:points.append(Vector((x,slope*.006,z)))
            for z in [zmin,zmax]:
                x=(z-intercept)/slope
                if xmin<x<xmax:points.append(Vector((x,slope*.006,z)))
            if len(points)!=2:continue
            a,b=points
            axis=(b-a).normalized()
            u=Vector((0,1,0))*.006
            v=axis.cross(u)
            start=len(verts)
            for point in [a,b]:
                for n in range(6):
                    angle=n*math.tau/6
                    verts.append(point+math.cos(angle)*u+math.sin(angle)*v)
            for n in range(6):faces.append((start+n,start+(n+1)%6,start+6+(n+1)%6,start+6+n))
    mesh=bpy.data.meshes.new('Regular diamond wire')
    mesh.from_pydata(verts,[],faces)
    mesh.materials.append(material)
    wire=bpy.data.objects.new('Repaired chain-link wire',mesh)
    bpy.context.scene.collection.objects.link(wire)
    for p in mesh.polygons:p.use_smooth=True
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    wire.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.join()
    assert H.triangles(obj)<=20000

def prepare(source_root,only=None):
    for name,settings in ASSETS.items():
        if only and name not in only:continue
        source=source_root/'artifacts/model-jobs'/('dockside-'+name)/'model.glb'
        assert source.exists(),source
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
        original_count=H.triangles(obj)
        assert 0<original_count<=20000,(name,original_count)
        lo,hi=H.bounds([obj])
        original_size=list(hi-lo)
        rotation=0
        if name=='wall-lamp':
            rotation=-90
            for v in obj.data.vertices:v.co=Matrix.Rotation(-math.pi/2,4,'Z')@v.co
        elif name not in {'quay-corner','dock-edge'} and (hi-lo).x<(hi-lo).y:
            rotation=90
            for v in obj.data.vertices:v.co=Matrix.Rotation(math.pi/2,4,'Z')@v.co
        lo,hi=H.bounds([obj])
        extent=hi-lo
        if 'size' in settings:size=settings['size']
        else:
            factor=settings['height']/extent.z if 'height' in settings else settings['width']/extent.x
            size=[c*factor for c in extent]
        H.fit(obj,size)
        if name=='chainlink-fence':repair_fence(obj)
        if name in {'guardrail','floodlight-mast'}:
            for v in obj.data.vertices:v.co=Matrix.Rotation(math.pi,4,'Z')@v.co
            rotation+=180
        original_maps=[]
        cap=1024 if 'budget' in settings else 2048
        for im in list(bpy.data.images):
            if im.type!='IMAGE':continue
            original_maps.append({'name':im.name,'size':list(im.size)})
            w,h=im.size
            if max(w,h)>cap:im.scale(round(w*cap/max(w,h)),round(h*cap/max(w,h)))
            im.pack()
        if 'budget' in settings:
            source_obj=obj
            obj=bpy.data.objects.new(name,source_obj.data.copy())
            bpy.context.scene.collection.objects.link(obj)
            H.reduce(obj,settings['budget'])
            H.fit(obj,size)
            H.bake_material(source_obj,obj,name)
            source_obj.hide_render=True
            source_obj.hide_set(True)
        obj.name=name
        obj['asset_id']='dockside-'+name
        obj['units']='meters'
        obj['pivot']='bottom center'
        result=H.export(DEST/(name+'.glb'),[obj])
        assert result['triangles_instanced']<=20000
        H.render([obj],QA/(name+'-front.png'),(-1.4,-2,1.15),size=(1200,1000))
        H.render([obj],QA/(name+'-back.png'),(1.4,2,1.15),size=(1200,1000))
        bpy.ops.wm.save_as_mainfile(filepath=str(QA/(name+'.blend')),compress=True)
        provider=json.loads(source.with_suffix('.glb.provenance.json').read_text())
        record={'id':name,**result,'units':'meters','up_axis':'Y',
            'dimensions_xyz_gltf':[size[0],size[2],size[1]],'pivot':'bottom center',
            'source_triangles':original_count,'source_dimensions_blender':original_size,
            'source_rotation_z_degrees':rotation,'original_maps':original_maps,
            'texture_cap':cap,'pbr_rebaked':'budget' in settings,
            'offline_repairs':['Regular geometry wire web; source frame retained'] if name=='chainlink-fence' else [],
            'provider':'Tripo3D','task_id':provider['task_id'],
            'credits_consumed':provider.get('credits_consumed'),
            'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'reference_sha256':provider['image_sha256'],'license_status':provider['license_status'],
            'request':{k:v for k,v in provider['request'].items() if k!='input'}}
        H.save_json(DEST/(name+'.asset.json'),record)
        shutil.copyfile(source_root/'artifacts/model-input-images'/('dockside-'+name)/'reference.json',
                        DEST/(name+'.reference.json'))
        print('PREPARED '+json.dumps(record),flush=True)
    records=[json.loads((DEST/(name+'.asset.json')).read_text()) for name in ASSETS if (DEST/(name+'.asset.json')).exists()]
    H.save_json(DEST/'manifest.json',{'assets':records,'credits_consumed':sum(r['credits_consumed'] for r in records),
        'scope':'Static scenery assets; no game placement, collision or runtime light setup'})

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,required=True)
    parser.add_argument('--only',nargs='+',choices=list(ASSETS))
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    DEST.mkdir(parents=True,exist_ok=True)
    QA.mkdir(parents=True,exist_ok=True)
    prepare(args.source_root,args.only)
