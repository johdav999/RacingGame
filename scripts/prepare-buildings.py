"""Offline Blender preparation of retained Tripo buildings; no provider calls.

blender --background --python scripts/prepare-buildings.py -- <asset-name>
"""
import bpy
import hashlib
import json
import struct
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
HEIGHTS = {'kairox-market-block': 24., 'kairox-service-depot': 10.,
           'kairox-transit-tower': 32., 'kairox-shop-arcade': 12.,
           'kairox-player-garage': 14., 'kairox-corner-apartments': 30.,
           'kairox-loading-warehouse': 11.}
name = sys.argv[sys.argv.index('--') + 1]
height = HEIGHTS[name]
source = ROOT / 'artifacts/model-jobs' / name / 'model.glb'
dest = ROOT / 'assets/runtime/models/buildings' / name
dest.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
points = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo = Vector(tuple(min(p[i] for p in points) for i in range(3)))
hi = Vector(tuple(max(p[i] for p in points) for i in range(3)))
origin = Vector(((lo.x+hi.x)/2, (lo.y+hi.y)/2, lo.z))
scale = height / (hi.z-lo.z)
before = 0
for obj in meshes:
    obj.data.calc_loop_triangles()
    before += len(obj.data.loop_triangles)
    matrix = obj.matrix_world.copy()
    for vertex in obj.data.vertices:
        vertex.co = (matrix @ vertex.co - origin) * scale
    obj.matrix_world.identity()
    obj.name = name
    obj.data.update()
    # Preserve provider geometry and UV seams. Aggressive collapse damaged
    # facade textures during QA; reduction needs a separately baked LOD.
points = [v.co.copy() for o in meshes for v in o.data.vertices]
lo = Vector(tuple(min(p[i] for p in points) for i in range(3)))
hi = Vector(tuple(max(p[i] for p in points) for i in range(3)))
origin = Vector(((lo.x+hi.x)/2, (lo.y+hi.y)/2, lo.z))
scale = height/(hi.z-lo.z)
for obj in meshes:
    for vertex in obj.data.vertices:
        vertex.co = (vertex.co-origin)*scale
    obj.data.update()
source_texture_sizes = [list(image.size) for image in bpy.data.images if image.size[0] and image.size[1]]
for image in bpy.data.images:
    if image.size[0] and image.size[1]:
        image.scale(1024, 1024)
        image.pack()
bpy.ops.object.select_all(action='DESELECT')
for obj in meshes:
    obj.select_set(True)
output = dest / (name + '.glb')
bpy.ops.export_scene.gltf(filepath=str(output), export_format='GLB',
    use_selection=True, export_yup=True, export_animations=False,
    export_cameras=False, export_lights=False)
raw = output.read_bytes()
size = struct.unpack_from('<I',raw,12)[0]
doc = json.loads(raw[20:20+size])
binary = raw[28+size:]
texture_sizes = []
for image in doc.get('images', []):
    view = doc['bufferViews'][image['bufferView']]
    data = binary[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']]
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Expected embedded PNG'
    dims = list(struct.unpack_from('>II',data,16))
    assert dims == [1024,1024], dims
    texture_sizes.append(dims)
triangles = sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
assert triangles == before, (triangles, before)
job = json.loads(source.with_suffix('.glb.provenance.json').read_text())
reference = json.loads((ROOT/'artifacts/model-input-images'/name/'provenance-v1.json').read_text(encoding='utf-8-sig'))
dimensions = (hi-lo)*scale
record = dict(asset=name, provider='Tripo3D', task_id=job['task_id'],
    credits_consumed=job.get('credits_consumed'), source_texture_sizes=source_texture_sizes,
    source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    reference_sha256=reference['sha256'], sha256=hashlib.sha256(raw).hexdigest(),
    bytes=len(raw), source_triangles=before, triangles=triangles,
    texture_sizes=texture_sizes, dimensions_xyz_metres=[dimensions.x,dimensions.z,dimensions.y],
    coordinates='+Y up; origin at bottom footprint centre; uniform scale',
    preparation='scripts/prepare-buildings.py; source geometry preserved, normalized scale and 1024px maps',
    collision='Not included; use separate simple trackside collision outside racing envelope',
    validation='Static Blender import and renders; not placed or tested in gameplay',
    license_status=job['license_status'])
(dest/(name+'.asset.json')).write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record))
