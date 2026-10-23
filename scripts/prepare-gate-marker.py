"""Author the precision navigation sign offline in Blender; no provider calls.

The previous generated source is retained in artifacts/model-jobs. This replaces
its runtime derivative with a texture-free, reproducible wayfinding asset.
"""
import bpy, hashlib, json, math, struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
dest = ROOT / 'assets/runtime/models/navigation/next-gate-beacon'
dest.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

def material(name, color, emission=False):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Metallic'].default_value = .45 if not emission else 0
    p.inputs['Roughness'].default_value = .42
    if emission:
        p.inputs['Emission'].default_value = (*color, 1)
        p.inputs['Emission Strength'].default_value = 1
    return m

graphite = material('Satin graphite housing', (.018, .026, .035))
amber = material('Amber illuminated face geometry', (1, .48, .045), True)

def prism(name, outline, back, front, mat, bevel=0):
    # Blender +Y faces approaching traffic (glTF -Z). Outline is in X/Z.
    n = len(outline)
    verts = [(x, y, z) for y in (back, front) for x, z in outline]
    faces = [tuple(range(n-1, -1, -1)), tuple(range(n, 2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces); mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False); bpy.ops.object.mode_set(mode='OBJECT')
    if bevel:
        mod = obj.modifiers.new('Machined edge', 'BEVEL'); mod.width = bevel; mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.select_set(False)
    return obj

def panel(name, w, low, high, cut, back, front, mat, bevel=0):
    outline = [(-w/2+cut, low), (w/2-cut, low), (w/2, low+cut),
               (w/2, high-cut), (w/2-cut, high), (-w/2+cut, high),
               (-w/2, high-cut), (-w/2, low+cut)]
    return prism(name, outline, back, front, mat, bevel)

# Deep feet overlap the measured beam; the face remains completely above it.
for x in (-1.85, 1.85):
    prism('Mounting foot', [(x-.23, 0), (x+.23, 0), (x+.23, .76), (x-.23, .76)], -.18, .18, graphite, .035)
panel('Chamfered enclosure', 5.6, .65, 2.45, .22, -.21, .21, graphite, .035)
panel('Continuous amber rim', 5.38, .76, 2.34, .17, .212, .231, amber)
panel('Recessed contrast face', 5.24, .83, 2.27, .14, .232, .244, graphite)

# Geometry avoids atlas blur and GL fallback emissive-mask differences.
# Blender's built-in font keeps this self-contained.
bpy.ops.object.text_add(location=(2.26, .256, 1.19), rotation=(math.pi/2, 0, math.pi))
label = bpy.context.object; label.name = 'NEXT legend'
label.data.body = 'NEXT'; label.data.size = 1.05; label.data.offset = .018
label.data.space_character = 1.05; label.data.extrude = .003
label.data.materials.append(amber)
bpy.ops.object.convert(target='MESH'); label.scale.x = 2.1 / label.dimensions.x
label.select_set(False)
prism('Legend divider', [(-.35, 1.04), (-.31, 1.04), (-.31, 2.06), (-.35, 2.06)], .248, .26, amber)
for z in (1.40, 1.94):
    x = -1.55
    prism('Downward chevron', [(x-.70, z+.30), (x, z-.24), (x+.70, z+.30),
          (x+.70, z+.06), (x, z-.48), (x-.70, z+.06)], .248, .28, amber)

bpy.ops.object.select_all(action='SELECT')
bpy.context.view_layer.objects.active = next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.ops.object.join()
obj = bpy.context.object; obj.name = 'Next gate sign'
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
output = dest / 'next-gate-beacon.glb'
bpy.ops.export_scene.gltf(filepath=str(output), export_format='GLB', use_selection=True,
    export_yup=True, export_animations=False, export_cameras=False, export_lights=False)
raw = output.read_bytes(); size = struct.unpack_from('<I', raw, 12)[0]
doc = json.loads(raw[20:20+size])
primitives = [p for m in doc['meshes'] for p in m['primitives']]
assert len(primitives)==2 and len(doc['materials'])==2
accessors = [doc['accessors'][p['attributes']['POSITION']] for p in primitives]
lo = [min(a['min'][i] for a in accessors) for i in range(3)]
hi = [max(a['max'][i] for a in accessors) for i in range(3)]
triangles = sum(doc['accessors'][p['indices']]['count']//3 for p in primitives)
record = dict(asset='next-gate-beacon', provider='Offline authored geometry',
    sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw), triangles=triangles,
    bounds_min=lo, bounds_max=hi, dimensions=[hi[i]-lo[i] for i in range(3)], materials=2,
    textures=0, coordinates='+Y up, bottom-centred feet, luminous face -Z',
    preparation='scripts/prepare-gate-marker.py; Blender 3.5; no external fonts or textures',
    mounting='Imported central beam top minus 0.25m; 0.65m support clearance')
(dest/'next-gate-beacon.asset.json').write_text(json.dumps(record, indent=2)+'\n')
(ROOT/'src/gate_marker_contract.hpp').write_text('''#pragma once
#include "engine/math.hpp"
namespace racing::gateart {
inline constexpr const char* name="next-gate-beacon";
inline constexpr const char* hash="%s";
inline constexpr unsigned triangles=%d;
inline const engine::Vec3 dimensions{%s};
}
''' % (record['sha256'], triangles, ','.join(f'{x:.6f}f' for x in record['dimensions'])))
print(json.dumps(record))
