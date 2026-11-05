"""Reimport final GLBs, verify their actual payloads, and render delivery evidence."""
import hashlib
import importlib.util
import json
import struct
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('prepare', ROOT/'scripts/prepare-dockside-containers.py')
prep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prep)
DEST = ROOT/'assets/runtime/models/dockside-containers'
QA = ROOT/'artifacts/dockside-containers'


def glb(path):
    blob=path.read_bytes()
    assert blob[:4]==b'glTF' and struct.unpack_from('<I',blob,4)[0]==2
    assert struct.unpack_from('<I',blob,8)[0]==len(blob)
    length=struct.unpack_from('<I',blob,12)[0]
    doc=json.loads(blob[20:20+length])
    bin_start=20+length+8
    return doc,blob[bin_start:]


def image_hash(doc,blob,texture):
    image=doc['images'][doc['textures'][texture['index']]['source']]
    view=doc['bufferViews'][image['bufferView']]
    start=view.get('byteOffset',0)
    return hashlib.sha256(blob[start:start+view['byteLength']]).hexdigest()


reports=[]
for name,job,dimensions,color in prep.SPECS:
    path=DEST/name/(name+'.glb')
    doc,blob=glb(path)
    record=json.loads((DEST/name/'asset.json').read_text())
    assert hashlib.sha256(path.read_bytes()).hexdigest()==record['sha256']
    count=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
    assert count==record['triangles'] and count<=20000
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    lo,hi=prep.bounds(obj)
    assert max(abs((hi-lo)[i]-dimensions[i]) for i in range(3))<.00002
    assert abs(lo.z)<.00002 and abs(lo.x+hi.x)<.00002 and abs(lo.y+hi.y)<.00002
    maps=[{'name':im.name,'size':list(im.size)} for im in bpy.data.images if im.type=='IMAGE']
    assert len(maps)==3 and all(max(im['size'])<=2048 for im in maps)
    mat=obj.data.materials[0]
    bsdf=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    actual=prep.pixels(bsdf.inputs['Base Color'].links[0].from_node.image)
    original=prep.pixels(bpy.data.images.load(str(DEST/name/'original-albedo.png')))
    mask=prep.pixels(bpy.data.images.load(str(DEST/name/'paint-mask.png')))[:,0]
    protected=float(np.max(np.abs(actual[mask==0,:3]-original[mask==0,:3])))
    assert protected<=1/255+.00001,protected
    scene,cam,center,extent=prep.studio(obj)
    prep.render_view(scene,cam,center,extent,(-1.25,-2,.9),QA/name/'exported-front.png')
    prep.render_view(scene,cam,center,extent,(1.25,2,.9),QA/name/'exported-back.png')
    reports.append({'name':name,'triangles':count,'dimensions_m_LWH':list(hi-lo),
                    'pivot':'bottom center','maps':maps,'protected_texel_max_error_after_glb':protected})

# Check an actual arbitrary-color export, including immutable geometry and PBR data.
base=DEST/'container-20ft/container-20ft.glb'
alternate=QA/'container-20ft/ochre.glb'
base_doc,base_bin=glb(base)
alt_doc,alt_bin=glb(alternate)
for index, accessor in enumerate(base_doc['accessors']):
    assert accessor==alt_doc['accessors'][index]
    bv=base_doc['bufferViews'][accessor['bufferView']]
    av=alt_doc['bufferViews'][accessor['bufferView']]
    assert bv['byteLength']==av['byteLength']
    assert base_bin[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']]==alt_bin[av.get('byteOffset',0):av.get('byteOffset',0)+av['byteLength']]
bm,am=base_doc['materials'][0],alt_doc['materials'][0]
assert image_hash(base_doc,base_bin,bm['normalTexture'])==image_hash(alt_doc,alt_bin,am['normalTexture'])
assert image_hash(base_doc,base_bin,bm['pbrMetallicRoughness']['metallicRoughnessTexture'])==image_hash(alt_doc,alt_bin,am['pbrMetallicRoughness']['metallicRoughnessTexture'])
assert image_hash(base_doc,base_bin,bm['pbrMetallicRoughness']['baseColorTexture'])!=image_hash(alt_doc,alt_bin,am['pbrMetallicRoughness']['baseColorTexture'])
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(alternate))
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bsdf=next(n for n in obj.data.materials[0].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
actual=prep.pixels(bsdf.inputs['Base Color'].links[0].from_node.image)
original=prep.pixels(bpy.data.images.load(str(DEST/'container-20ft/original-albedo.png')))
mask=prep.pixels(bpy.data.images.load(str(DEST/'container-20ft/paint-mask.png')))[:,0]
protected=float(np.max(np.abs(actual[mask==0,:3]-original[mask==0,:3])))
assert protected<=1/255+.00001
assert float(np.mean(np.abs(actual[mask>.99,:3]-original[mask>.99,:3])))>.1
scene,cam,center,extent=prep.studio(obj)
prep.render_view(scene,cam,center,extent,(-1.25,-2,.9),QA/'container-20ft/exported-ochre.png')
report={'models':reports,'alternate_color':{'hex':'#C99B45','geometry_identical':True,
        'normal_and_orm_identical':True,'albedo_changed':True,'protected_texel_max_error_after_glb':protected}}
(QA/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('VERIFIED '+json.dumps(report),flush=True)

# One shared view at real relative scale, using the delivered GLBs themselves.
bpy.ops.wm.read_factory_settings(use_empty=True)
objects=[]
for row,(name,_,_,_) in enumerate(prep.SPECS):
    before=set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(DEST/name/(name+'.glb')))
    obj=next(o for o in bpy.context.scene.objects if o not in before and o.type=='MESH')
    for v in obj.data.vertices:
        v.co.y+=row*6
    objects.append(obj)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join()
obj=bpy.context.object
scene,cam,center,extent=prep.studio(obj)
scene.render.resolution_x=1600
scene.render.resolution_y=1100
prep.render_view(scene,cam,center,extent,(-1.2,-2,2.0),QA/'container-lineup.png')
