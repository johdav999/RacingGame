"""Prepare Tripo containers and export arbitrary masked paint colors with Blender 3.5+.

blender --background --python scripts/prepare-dockside-containers.py -- --source-root REPO
blender --background --python scripts/prepare-dockside-containers.py -- --recolor \
    assets/runtime/models/dockside-containers/container-20ft/container-20ft.glb \
    --color '#D5A34A' --output custom.glb
"""
import argparse
import hashlib
import json
import math
import struct
import sys
import tempfile
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[1]
SPECS = [
    ('container-20ft', 'dockside-container-20ft-v1', (6.058, 2.438, 2.591), '245D88'),
    ('container-40ft', 'dockside-container-40ft', (12.192, 2.438, 2.591), '913E34'),
    ('container-40ft-highcube', 'dockside-container-40ft-highcube', (12.192, 2.438, 2.896), '3E6854'),
]


def linear(value):
    value = np.asarray(value)
    return np.where(value <= .04045, value / 12.92, ((value + .055) / 1.055) ** 2.4)


def srgb(value):
    value = np.maximum(value, 0)
    return np.where(value <= .0031308, value * 12.92, 1.055 * value ** (1 / 2.4) - .055)


def color_value(value):
    value = value.lstrip('#')
    if len(value) != 6 or any(c not in '0123456789abcdefABCDEF' for c in value):
        raise ValueError('Color must be a six-digit sRGB hex value')
    return linear([int(value[i:i+2], 16) / 255 for i in (0, 2, 4)])


def pixels(image):
    data = np.empty(image.size[0] * image.size[1] * 4, dtype=np.float32)
    image.pixels.foreach_get(data)
    return data.reshape((-1, 4))


def texture(name, data, size, path, noncolor=False):
    image = bpy.data.images.new(name, width=size[0], height=size[1], alpha=True)
    image.colorspace_settings.name = 'Non-Color' if noncolor else 'sRGB'
    image.pixels.foreach_set(np.asarray(data, dtype=np.float32).reshape(-1))
    image.filepath_raw = str(path)
    image.file_format = 'PNG'
    image.save()
    image.pack()
    return image


def bounds(obj):
    return (Vector([min(v.co[i] for v in obj.data.vertices) for i in range(3)]),
            Vector([max(v.co[i] for v in obj.data.vertices) for i in range(3)]))


def shader_image(mat, name, image, location):
    node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.name = node.label = name
    node.image = image
    node.location = location
    return node


def masked_material(mat, folder, color):
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    original = bsdf.inputs['Base Color'].links[0].from_node
    original.name = original.label = 'Original finish (protected hardware and wear)'
    im = original.image
    im.name = 'Original finish'
    im.filepath_raw = str(folder / 'original-albedo.png')
    im.file_format = 'PNG'
    im.save()
    im.pack()
    data = pixels(im)
    r, g, b = data[:, 0], data[:, 1], data[:, 2]
    # A blue-painted reference provides a chromatic mask, independent of geometry.
    # Neutral steel, dark gaskets and warm rust stay outside the mask.
    strength = np.minimum((b-r) / np.maximum(b, .02), (b-g) / np.maximum(b, .02) * 2)
    t = np.clip((strength - .10) / .24, 0, 1)
    mask = t*t*(3-2*t)
    assert np.count_nonzero(mask > .9) > len(mask) * .1, 'Insufficient blue paint coverage'
    # Painted enamel is dielectric and moderately rough; exposed hardware keeps
    # the provider's metallic/roughness values. This also removes chrome-like roofs.
    orm_node = bsdf.inputs['Metallic'].links[0].from_node.inputs[0].links[0].from_node
    orm = pixels(orm_node.image)
    assert tuple(orm_node.image.size) == tuple(im.size)
    orm[:, 1] = orm[:, 1]*(1-mask) + np.maximum(orm[:, 1], .57)*mask
    orm[:, 2] = orm[:, 2]*(1-mask) + .035*mask
    orm_node.image = texture('Painted steel ORM', orm, im.size, folder/'painted-steel-orm.png', True)
    brightest = np.max(data[:, :3], axis=1)
    reference = float(np.percentile(brightest[mask > .9], 85))
    shade = np.clip(linear(brightest) / max(float(linear(reference)), .02), .025, 1.15)
    mask_rgba = np.ones_like(data)
    mask_rgba[:, :3] = mask[:, None]
    shade_rgba = np.ones_like(data)
    shade_rgba[:, :3] = srgb(shade[:, None])
    mask_image = texture('Paint mask', mask_rgba, im.size, folder / 'paint-mask.png', True)
    shade_image = texture('Paint shading', shade_rgba, im.size, folder / 'paint-shading.png')
    paint_graph(mat, im, mask_image, shade_image, color)
    return {'paint_coverage': float(np.mean(mask)), 'protected_texels': int(np.count_nonzero(mask == 0)),
            'paint_reference_srgb': reference}


def paint_graph(mat, original_image, mask_image, shade_image, color):
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
    original = bsdf.inputs['Base Color'].links[0].from_node
    original.name = original.label = 'Original finish (protected hardware and wear)'
    original.image = original_image
    mask_image.colorspace_settings.name = 'Non-Color'
    mask_node = shader_image(mat, 'Paint mask', mask_image, (-750, 300))
    shade_node = shader_image(mat, 'Paint shading', shade_image, (-750, -100))
    tint = nodes.new('ShaderNodeRGB')
    tint.name = tint.label = 'Paint Color'
    tint.location = (-750, -350)
    tint.outputs[0].default_value = (*color_value(color), 1)
    multiply = nodes.new('ShaderNodeMixRGB')
    multiply.name = 'Paint times shading'
    multiply.blend_type = 'MULTIPLY'
    multiply.inputs[0].default_value = 1
    multiply.location = (-450, -100)
    links.new(shade_node.outputs['Color'], multiply.inputs[1])
    links.new(tint.outputs[0], multiply.inputs[2])
    mix = nodes.new('ShaderNodeMixRGB')
    mix.name = 'Masked paint finish'
    mix.location = (-200, 100)
    links.new(mask_node.outputs['Color'], mix.inputs[0])
    links.new(original.outputs['Color'], mix.inputs[1])
    links.new(multiply.outputs[0], mix.inputs[2])
    links.new(mix.outputs[0], bsdf.inputs['Base Color'])
    mat.name = 'Dockside_PaintableSteel'
    mat['paint_control'] = 'Paint Color'
    mat['paint_mask'] = 'paint-mask.png'
    mat['paint_method'] = 'Mix protected original with shaded paint in linear color space'


def export_color(obj, color, output):
    """Bake just the color math into an ordinary glTF PBR albedo, without a light bake."""
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    rgb = color_value(color)
    restores = []
    protected_errors = []
    for mat in obj.data.materials:
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        if 'Paint Color' not in nodes:
            continue
        nodes['Paint Color'].outputs[0].default_value = (*rgb, 1)
        original = nodes['Original finish (protected hardware and wear)'].image
        base = pixels(original)
        mask = pixels(nodes['Paint mask'].image)[:, :1]
        shade = linear(pixels(nodes['Paint shading'].image)[:, :3])
        out = base.copy()
        out[:, :3] = np.clip(srgb(linear(base[:, :3]) * (1-mask) + shade * rgb * mask), 0, 1)
        out[mask[:, 0] == 0] = base[mask[:, 0] == 0]
        protected_errors.append(float(np.max(np.abs(out[mask[:, 0] == 0] - base[mask[:, 0] == 0]))))
        image = bpy.data.images.new('Paint '+color.lstrip('#'), width=original.size[0], height=original.size[1], alpha=True)
        image.pixels.foreach_set(out.astype(np.float32).reshape(-1))
        image.pack()
        bsdf = next(n for n in nodes if n.type == 'BSDF_PRINCIPLED')
        old = bsdf.inputs['Base Color'].links[0].from_socket
        node = shader_image(mat, 'GLB baked paint', image, (-200, 500))
        links.new(node.outputs['Color'], bsdf.inputs['Base Color'])
        restores.append((mat, bsdf, old, node, image))
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=str(output), export_format='GLB', use_selection=True,
                             export_yup=True, export_animations=False, export_cameras=False,
                             export_lights=False, export_extras=True)
    for mat, bsdf, old, node, image in restores:
        mat.node_tree.links.new(old, bsdf.inputs['Base Color'])
        mat.node_tree.nodes.remove(node)
        bpy.data.images.remove(image)
    assert all(error == 0 for error in protected_errors)
    return {'protected_texel_max_error_before_encoding': max(protected_errors, default=0)}


def replace_glb_albedo(source, painted, output):
    """Replace only the embedded albedo; keep original geometry bytes untouched.

    A Blender import/export can reorder vertices or recompute split normals.
    Portable recoloring therefore patches the source GLB instead of shipping
    that re-exported geometry. Repack buffer views to avoid retaining old albedo.
    """
    def read(path):
        raw=Path(path).read_bytes()
        length=struct.unpack_from('<I',raw,12)[0]
        return json.loads(raw[20:20+length]),raw[28+length:]
    def color_image(doc):
        index=doc['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['index']
        return doc['images'][doc['textures'][index]['source']]
    doc,body=read(source)
    painted_doc,painted_body=read(painted)
    old_image,new_image=color_image(doc),color_image(painted_doc)
    new_view=painted_doc['bufferViews'][new_image['bufferView']]
    start=new_view.get('byteOffset',0)
    replacement=painted_body[start:start+new_view['byteLength']]
    packed=bytearray()
    for index,view in enumerate(doc['bufferViews']):
        assert view.get('buffer',0)==0
        start=view.get('byteOffset',0)
        data=replacement if index==old_image['bufferView'] else body[start:start+view['byteLength']]
        packed.extend(b'\0'*((-len(packed))%4))
        view['byteOffset'],view['byteLength']=len(packed),len(data)
        packed.extend(data)
    old_image['mimeType']=new_image['mimeType']
    old_image['name']=new_image.get('name','Paint')
    doc['buffers'][0]['byteLength']=len(packed)
    packed.extend(b'\0'*((-len(packed))%4))
    encoded=json.dumps(doc,separators=(',',':')).encode()
    encoded+=b' '*((-len(encoded))%4)
    result=struct.pack('<4sII',b'glTF',2,28+len(encoded)+len(packed))
    result+=struct.pack('<I4s',len(encoded),b'JSON')+encoded
    result+=struct.pack('<I4s',len(packed),b'BIN\0')+packed
    with Path(output).open('xb') as stream:
        stream.write(result)


def studio(obj):
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE'
    scene.eevee.use_gtao = True
    scene.eevee.gtao_distance = .4
    scene.eevee.taa_render_samples = 48
    scene.render.resolution_x = 1400
    scene.render.resolution_y = 800
    scene.render.resolution_percentage = 100
    scene.world = bpy.data.worlds.new('Container studio')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value = (.16, .18, .22, 1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value = .7
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'Medium High Contrast'
    lo, hi = bounds(obj)
    center, extent = (lo+hi)/2, max(hi-lo)
    for label, direction, power in [('Key',(-1,-2,3),80),('Fill',(1,-1,2),45),('Rim',(0,2,3),90)]:
        light = bpy.data.lights.new(label, 'AREA')
        light.energy, light.size = power*extent*extent, extent*1.5
        ob = bpy.data.objects.new(label, light)
        scene.collection.objects.link(ob)
        ob.location = center + Vector(direction)*extent
        ob.rotation_euler = (center-ob.location).to_track_quat('-Z','Y').to_euler()
    cam = bpy.data.objects.new('Review camera', bpy.data.cameras.new('Review camera'))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type, cam.data.ortho_scale = 'ORTHO', extent*1.15
    return scene, cam, center, extent


def render_view(scene, cam, center, extent, direction, path):
    cam.location = center + Vector(direction)*extent
    cam.rotation_euler = (center-cam.location).to_track_quat('-Z','Y').to_euler()
    rotation = cam.rotation_euler.to_matrix().transposed()
    obj = next(o for o in scene.objects if o.type == 'MESH')
    projected = [rotation @ (obj.matrix_world @ v.co-center) for v in obj.data.vertices]
    width = max(v.x for v in projected)-min(v.x for v in projected)
    height = max(v.y for v in projected)-min(v.y for v in projected)
    cam.data.ortho_scale = max(width,height*scene.render.resolution_x/scene.render.resolution_y)*1.12
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def prepare(source_root, only=None):
    records = []
    for name, job, dimensions, color in SPECS:
        if only and name != only:
            continue
        source = source_root / 'artifacts/model-jobs' / job / 'model.glb'
        dest = ROOT / 'assets/runtime/models/dockside-containers' / name
        qa = ROOT / 'artifacts/dockside-containers' / name
        dest.mkdir(parents=True, exist_ok=True)
        qa.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(source))
        meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        bpy.ops.object.select_all(action='DESELECT')
        for obj in meshes:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
        obj = bpy.context.object
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        obj.name = name
        obj.data.calc_loop_triangles()
        triangles = len(obj.data.loop_triangles)
        assert 0 < triangles <= 20000, triangles
        lo, hi = bounds(obj)
        source_dimensions = list(hi-lo)
        if (hi-lo).y > (hi-lo).x:
            for v in obj.data.vertices:
                v.co = Matrix.Rotation(math.pi/2, 4, 'Z') @ v.co
        lo, hi = bounds(obj)
        size = hi-lo
        origin = Vector(((lo.x+hi.x)/2, (lo.y+hi.y)/2, lo.z))
        for v in obj.data.vertices:
            v.co = Vector([(v.co[i]-origin[i])*dimensions[i]/size[i] for i in range(3)])
        obj.data.update()
        assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
        original_maps = []
        for im in list(bpy.data.images):
            if im.type != 'IMAGE':
                continue
            original_maps.append({'name':im.name, 'size':list(im.size)})
            w,h = im.size
            if max(w,h)>2048:
                im.scale(round(w*2048/max(w,h)), round(h*2048/max(w,h)))
            im.pack()
        assert len(obj.data.materials) == 1, 'Review unexpected material layout'
        mask_qa = masked_material(obj.data.materials[0], dest, color)
        obj['dimensions_m_LWH'] = list(dimensions)
        obj['pivot'] = 'Bottom center; long axis X; glTF Y up'
        color_qa = export_color(obj, color, dest / (name+'.glb'))
        provider = json.loads(source.with_suffix('.glb.provenance.json').read_text())
        blob = (dest / (name+'.glb')).read_bytes()
        doc = json.loads(blob[20:20+struct.unpack_from('<I',blob,12)[0]])
        actual = sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
        assert actual == triangles
        assert all('normalTexture' in m and 'metallicRoughnessTexture' in m['pbrMetallicRoughness'] for m in doc['materials'])
        record = {'name':name,'dimensions_m_LWH':dimensions,'triangles':triangles,
                  'default_color':'#'+color,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest(),
                  'provider':'Tripo3D','task_id':provider['task_id'],
                  'credits_consumed':provider.get('credits_consumed'),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                  'reference_sha256':provider['image_sha256'],'original_maps':original_maps,
                  'runtime_texture_max_dimension':2048,'source_dimensions_blender':source_dimensions,
                  'paint':{**mask_qa,**color_qa},'license_status':provider['license_status'],
                  'qa':'Finite geometry, measured GLB triangles, exact dimensions, normal map retained, paint-only dielectric/roughness adjustment, protected texels unchanged before encoding. Studio renders; no in-game integration.'}
        (dest/'asset.json').write_text(json.dumps(record,indent=2)+'\n')
        records.append(record)
        scene, cam, center, extent = studio(obj)
        render_view(scene,cam,center,extent,(-1.25,-2,.9),qa/'front.png')
        render_view(scene,cam,center,extent,(1.25,2,.9),qa/'back.png')
        bpy.ops.wm.save_as_mainfile(filepath=str(qa/(name+'.blend')))
        for label, test_color in [('ochre','C99B45'),('ivory','CFD1C6')]:
            obj.data.materials[0].node_tree.nodes['Paint Color'].outputs[0].default_value=(*color_value(test_color),1)
            render_view(scene,cam,center,extent,(-1.25,-2,.9),qa/(label+'.png'))
        print('CONTAINER '+json.dumps(record),flush=True)
    manifest = ROOT/'assets/runtime/models/dockside-containers/manifest.json'
    if not only:
        manifest.write_text(json.dumps({'assets':records,'total_triangles':sum(r['triangles'] for r in records)},indent=2)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=ROOT)
    parser.add_argument('--only',choices=[s[0] for s in SPECS])
    parser.add_argument('--recolor',type=Path)
    parser.add_argument('--color',default='C99B45')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    if args.recolor:
        if not args.output or args.output.suffix.lower()!='.glb':
            parser.error('--recolor requires --output ending in .glb')
        if args.output.exists():
            parser.error('Refusing to overwrite an existing export')
        color_value(args.color)
        if args.recolor.suffix.lower() == '.blend':
            bpy.ops.wm.open_mainfile(filepath=str(args.recolor.resolve()))
        elif args.recolor.suffix.lower() == '.glb':
            folder=args.recolor.resolve().parent
            for filename in ['original-albedo.png','paint-mask.png','paint-shading.png']:
                if not (folder/filename).is_file():
                    parser.error('GLB recoloring requires its original-albedo, paint-mask and paint-shading PNG sidecars')
            bpy.ops.wm.read_factory_settings(use_empty=True)
            bpy.ops.import_scene.gltf(filepath=str(args.recolor.resolve()))
            obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
            paint_graph(obj.data.materials[0],
                        bpy.data.images.load(str(folder/'original-albedo.png')),
                        bpy.data.images.load(str(folder/'paint-mask.png')),
                        bpy.data.images.load(str(folder/'paint-shading.png')),args.color)
        else:
            parser.error('--recolor accepts a prepared .glb or editable .blend')
        obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
        if args.recolor.suffix.lower()=='.glb':
            args.output.parent.mkdir(parents=True,exist_ok=True)
            with tempfile.TemporaryDirectory(prefix='container-paint-',dir=args.output.parent) as temporary:
                intermediate=Path(temporary)/'paint.glb'
                result=export_color(obj,args.color,intermediate)
                replace_glb_albedo(args.recolor,intermediate,args.output)
            result['source_geometry_preserved_exactly']=True
        else:
            result=export_color(obj,args.color,args.output)
        print(json.dumps(result))
    else:
        prepare(args.source_root.resolve(),args.only)


if __name__ == '__main__':
    main()
