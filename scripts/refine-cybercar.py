"""Deterministic offline rear rebuild. Keeps the source GLB and wheel streams intact.

Source coordinates: +X rear, +Y up, +Z left. Requires Pillow and Blender 3.5;
no provider calls. The small lofts below are the editable geometry source,
with bevels, weighted normals and UV/material baking in bake-cybercar-rear.py.
"""
import copy
import hashlib
import io
import json
import math
from pathlib import Path
import struct
import subprocess
import os
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'assets/runtime/models/cybercar1'
SOURCE = DEST / 'cybercar1_parts_textured.glb'
OUTPUT = DEST / 'cybercar1_rear_refined.glb'


def main():
    raw = SOURCE.read_bytes()
    size = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20+size])
    original = copy.deepcopy(doc)
    binary = bytearray(raw[28+size:])
    nodes, meshes = [], []
    wheel_evidence = []

    def view(data):
        binary.extend(b'\0' * (-len(binary) % 4))
        index = len(doc['bufferViews'])
        doc['bufferViews'].append({'buffer':0,'byteOffset':len(binary),'byteLength':len(data)})
        binary.extend(data)
        return index

    def accessor(values, width, integer=False):
        kind = 'I' if integer else 'f'
        data = b''.join(struct.pack('<'+kind*width,*v) for v in values)
        a = {'bufferView':view(data),'componentType':5125 if integer else 5126,
             'count':len(values),'type':{1:'SCALAR',2:'VEC2',3:'VEC3'}[width]}
        if width == 3 and not integer:
            a.update(min=[min(v[i] for v in values) for i in range(3)],
                     max=[max(v[i] for v in values) for i in range(3)])
        doc['accessors'].append(a)
        return len(doc['accessors'])-1

    def read(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        width = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        fmt = '<'+{5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]*width
        start = v.get('byteOffset',0)+a.get('byteOffset',0)
        stride = v.get('byteStride',struct.calcsize(fmt))
        return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]

    def material(name, color, roughness, metal=0, emission=None):
        m = {'name':name,'pbrMetallicRoughness':{'baseColorFactor':[*color,1],
            'metallicFactor':metal,'roughnessFactor':roughness}}
        if emission: m['emissiveFactor'] = emission
        doc['materials'].append(m)
        return len(doc['materials'])-1

    # Remove the baked chrome contrast from body albedo, retaining dark panel detail.
    source_texture = original['textures'][original['materials'][0]['pbrMetallicRoughness']['baseColorTexture']['index']]
    im = original['images'][source_texture['source']]
    bv = original['bufferViews'][im['bufferView']]
    albedo = Image.open(io.BytesIO(binary[bv.get('byteOffset',0):bv.get('byteOffset',0)+bv['byteLength']])).convert('RGB')
    gray = ImageOps.grayscale(albedo.resize((2048,2048),Image.Resampling.LANCZOS))
    # Compression above mid-gray suppresses baked highlights, without whitening glass/vents.
    gray = gray.point([round(255*(.018+.36*(i/255)**.72)) for i in range(256)])
    toned = Image.merge('RGB', tuple(gray.point([round(i*k) for i in range(256)]) for k in (.83,.92,1.0)))
    data = io.BytesIO(); toned.save(data,format='PNG')
    doc['images'].append({'name':'Vektor satin body albedo','mimeType':'image/png','bufferView':view(data.getvalue())})
    doc['textures'].append({'source':len(doc['images'])-1})
    body = material('Vektor satin gunmetal / retained shell',(1,1,1),.40,.42)
    doc['materials'][body]['pbrMetallicRoughness']['baseColorTexture'] = {'index':len(doc['textures'])-1}
    # The old tangent normal contains buckled-panel relief. Do not carry it onto the body.
    satin = material('Vektor satin gunmetal / rebuilt rear',(.12,.15,.19),.40,.42)
    edge = material('Brushed titanium bevels',(.36,.43,.49),.27,.85)
    dark = material('Graphite vents and aero',(.009,.016,.024),.86,.05)
    glass = material('Smoked rear glass',(.018,.045,.065),.12,0)
    red = material('Red running light lenses',(.58,.009,.012),.34,.0,(.8,.008,.012))
    wheel_material = copy.deepcopy(original['materials'][0])
    wheel_material['name'] = 'Original wheel PBR / preserved'
    doc['materials'].append(wheel_material)
    wheels = len(doc['materials'])-1

    def add(name, positions, normals, uvs, indices, mat):
        attrs = {'POSITION':accessor(positions,3),'NORMAL':accessor(normals,3),'TEXCOORD_0':accessor(uvs,2)}
        nodes.append({'name':name,'mesh':len(meshes)})
        meshes.append({'name':name,'primitives':[{'attributes':attrs,'indices':accessor([(i,) for i in indices],1,True),'material':mat}]})

    def retain(node, mesh):
        node = copy.deepcopy(node); node['mesh'] = len(meshes)
        nodes.append(node); meshes.append(copy.deepcopy(mesh))

    def clip(poly, axis, value, less=True):
        out = []
        for a,b in zip(poly,poly[1:]+poly[:1]):
            da,db = a[axis]-value,b[axis]-value
            if not less: da,db=-da,-db
            if da <= 0: out.append(a)
            if (da <= 0) != (db <= 0):
                t = da/(da-db)
                out.append(tuple(x+(y-x)*t for x,y in zip(a,b)))
        return out

    removed = []
    for node in original['nodes']:
        if 'mesh' not in node: continue
        mesh = original['meshes'][node['mesh']]
        p = mesh['primitives'][0]
        if node['name'].startswith('wheel_'):
            retain(node,mesh)
            meshes[-1]['primitives'][0]['material'] = wheels
            # Hash unpacked streams plus pivot: exact identity survives the preparation.
            streams = {k:read(v) for k,v in p['attributes'].items()}
            streams['indices'] = read(p['indices'])
            wheel_evidence.append({'name':node['name'],'sha256':hashlib.sha256(json.dumps(streams,sort_keys=True).encode()).hexdigest(),
                                   'translation':node['translation'],'radius':node['extras']['radius_source_units']})
            continue
        if node['name'] in ('car_part_02','car_part_06','car_part_07','car_part_10','car_part_14','car_part_15'):
            removed.append(node['name']); continue
        if node['name'] not in ('car_part_00','car_part_01'):
            retain(node,mesh); meshes[-1]['primitives'][0]['material'] = body
            continue
        pos,norm,uv = [read(p['attributes'][a]) for a in ('POSITION','NORMAL','TEXCOORD_0')]
        idx = [i[0] for i in read(p['indices'])]
        positions,normals,uvs,indices = [],[],[],[]
        for i in range(0,len(idx),3):
            poly = [tuple(pos[j])+tuple(norm[j])+tuple(uv[j]) for j in idx[i:i+3]]
            # Keep rear wheel arches and lower side skirts; cut the old tail cleanly.
            if node['name'] == 'car_part_01':
                polygons=[clip(poly,0,.106)]
            else:
                poly = clip(poly,0,.392)
                polygons=[clip(poly,0,.17),clip(clip(poly,0,.17,False),1,.184)]
            for poly in polygons:
                for j in range(1,len(poly)-1):
                    for v in (poly[0],poly[j],poly[j+1]):
                        indices.append(len(positions)); positions.append(v[:3])
                        length = math.sqrt(sum(n*n for n in v[3:6])) or 1
                        normals.append(tuple(n/length for n in v[3:6])); uvs.append(v[6:8])
        add(node['name'],positions,normals,uvs,indices,body)

    # Author planar faces with intentional split normals. New pieces have no baked damage.
    groups = {}
    pieces = []
    def faces(mat, vertices, polygons):
        pieces.append(dict(material=mat,vertices=vertices,polygons=polygons))
        positions,normals,uvs,indices = groups.setdefault(mat,([],[],[],[]))
        for face in polygons:
            a,b,c = [vertices[i] for i in face[:3]]
            u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
            n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
            length=math.sqrt(sum(x*x for x in n))
            if length<1e-10: continue
            n=tuple(x/length for x in n)
            start=len(positions)
            for i in face:
                positions.append(vertices[i]); normals.append(n); uvs.append((0,0))
            for i in range(1,len(face)-1): indices.extend((start,start+i,start+i+1))

    def loft(mat, sections):
        # Cross sections are clockwise in (Y,Z) as viewed along +X.
        vertices=[(x,y,z) for x,ring in sections for y,z in ring]
        count=len(sections[0][1]); polygons=[]
        polygons.append(tuple(reversed(range(count))))
        for s in range(len(sections)-1):
            for i in range(count):
                j=(i+1)%count
                polygons.append((s*count+i,s*count+j,(s+1)*count+j,(s+1)*count+i))
        polygons.append(tuple((len(sections)-1)*count+i for i in range(count)))
        # Determine outward winding for this convex loft from each face's centre.
        center=tuple(sum(p[a] for p in vertices)/len(vertices) for a in range(3))
        fixed=[]
        for f in polygons:
            a,b,c=[vertices[i] for i in f[:3]]
            u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
            n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
            mid=[sum(vertices[i][a] for i in f)/len(f)-center[a] for a in range(3)]
            fixed.append(tuple(reversed(f)) if sum(n[a]*mid[a] for a in range(3))<0 else f)
        faces(mat,vertices,fixed)

    def ring(bottom,top,width,bevel=.006,offset=0):
        return [(bottom,-width+bevel+offset),(bottom,width-bevel+offset),
                (bottom+bevel,width+offset),(top-bevel,width+offset),(top,width-bevel+offset),
                (top,-width+bevel+offset),(top-bevel,-width+offset),(bottom+bevel,-width+offset)]

    def box(mat,x0,x1,y0,y1,width,bevel=.002,offset=0):
        r=ring(y0,y1,width,min(bevel,(y1-y0)*.35),offset)
        loft(mat,[(x0,r),(x1,r)])

    # Tail body fits the retained quarter panels, ending in a chamfered rear face.
    loft(satin,[(.386,ring(.06,.201,.237,.012)),(.465,ring(.065,.202,.24,.014)),
                (.488,ring(.085,.186,.224,.012))])
    # Shoulder panels flow from the narrow roof buttresses to the broad rear quarters.
    for sign in (-1,1):
        vertices=[(.105,.278,sign*.125),(.105,.271,sign*.151),(.17,.183,sign*.243),
                  (.392,.183,sign*.238),(.392,.206,sign*.19),(.362,.219,sign*.169)]
        faces(satin,vertices,[(0,1,2),(0,2,3),(0,3,4),(0,4,5)] if sign>0 else
              [(2,1,0),(3,2,0),(4,3,0),(5,4,0)])
        # Slim brushed rails create a controlled highlight along the rear glass.
        loft(edge,[(.108,ring(.273,.28,.004,.001,sign*.126)),
                   (.36,ring(.213,.220,.004,.001,sign*.17))])
    # Close the transition to the retained roof rather than leaving the source frame's hole.
    loft(satin,[(-.046,ring(.281,.292,.126,.003)),(.052,ring(.279,.291,.127,.003)),
                (.114,ring(.272,.282,.129,.002))])
    # Dark rear window closes the old holes, with seven evenly spaced actual louvers.
    loft(glass,[(.11,ring(.264,.273,.121,.002)),(.367,ring(.201,.211,.17,.002))])
    for i in range(7):
        x=.133+i*.032
        y=.275-(x-.11)*.242
        width=.122+(x-.11)*.188
        loft(dark,[(x,ring(y-.007,y,width,.002)),(x+.018,ring(y-.010,y-.003,width+.003,.002))])
    box(satin,.365,.402,.193,.210,.191,.003)
    # Low wing, two pylons and chamfered end plates; retains the original spoiler profile.
    for sign in (-1,1):
        box(dark,.414,.432,.195,.239,.007,.002,sign*.151)
        box(satin,.402,.463,.233,.248,.006,.003,sign*.211)
    loft(satin,[(.401,ring(.235,.242,.209,.002)),(.455,ring(.241,.249,.209,.002))])
    box(edge,.454,.459,.242,.248,.202,.001)
    # Black light recess and twin continuous red blades, with angular outer returns.
    box(dark,.486,.492,.151,.184,.219,.006)
    for sign in (-1,1):
        box(red,.492,.494,.169,.176,.098,.002,sign*.106)
        box(red,.490,.494,.157,.174,.006,.002,sign*.207)
    # Bumper seam, inset bumper pad, diffuser and five vertical strakes.
    box(dark,.485,.490,.127,.133,.215,.002)
    box(satin,.483,.491,.098,.125,.179,.006)
    loft(dark,[(.388,ring(.045,.068,.212,.006)),(.484,ring(.052,.091,.211,.009)),
               (.495,ring(.056,.082,.199,.006))])
    for z in (-.16,-.08,0,.08,.16):
        loft(dark,[(.414,ring(.035,.062,.002,.0005,z)),(.494,ring(.041,.078,.002,.0005,z))])
    # Blender processes only the authored rear. Original streams never pass
    # through an exporter, preserving every wheel/pivot and source part ordering.
    work=ROOT/'artifacts/cybercar-material-pass';work.mkdir(parents=True,exist_ok=True)
    spec=dict(pieces=pieces,materials={str(m):doc['materials'][m] for m in groups},
              families={str(m):label for m,label in [(satin,'bodywork'),(edge,'trim'),(dark,'aero'),(glass,'glass'),(red,'lights')]})
    (work/'source.json').write_text(json.dumps(spec))
    blender=os.environ.get('BLENDER_EXECUTABLE','C:/Program Files/Blender Foundation/Blender 3.5/blender.exe')
    subprocess.run([blender,'--background','--python-exit-code','1','--python',str(ROOT/'scripts/bake-cybercar-rear.py'),'--',str(work)],check=True)
    prepared=json.loads((work/'prepared.json').read_text())
    maps={}
    for semantic in ('base-color','metallic-roughness'):
        data=(work/(semantic+'.png')).read_bytes()
        doc['images'].append({'name':'Vektor rear atlas '+semantic,'mimeType':'image/png','bufferView':view(data)})
        doc['textures'].append({'source':len(doc['images'])-1})
        maps[semantic]=len(doc['textures'])-1
    for mat in groups:
        surface=prepared[str(mat)]
        p,n,u,i=(surface[k] for k in ('positions','normals','uvs','indices'))
        pbr=doc['materials'][mat]['pbrMetallicRoughness']
        pbr.update(baseColorFactor=[1,1,1,1],metallicFactor=1,roughnessFactor=1,
                   baseColorTexture={'index':maps['base-color']},metallicRoughnessTexture={'index':maps['metallic-roughness']})
        add('rear_'+{satin:'bodywork',edge:'trim',dark:'aero',glass:'glass',red:'lights'}[mat],p,n,u,i,mat)

    doc.update(nodes=nodes,meshes=meshes,scenes=[{'nodes':list(range(len(nodes)))}],scene=0,
               buffers=[{'byteLength':len(binary)}])
    encoded=json.dumps(doc,separators=(',',':')).encode(); encoded+=b' '*(-len(encoded)%4)
    binary.extend(b'\0'*(-len(binary)%4))
    result=b'glTF'+struct.pack('<II',2,28+len(encoded)+len(binary))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+struct.pack('<I4s',len(binary),b'BIN\0')+binary
    OUTPUT.write_bytes(result)
    triangles=sum(doc['accessors'][p['indices']]['count']//3 for m in meshes for p in m['primitives'])
    record={'asset':'Vektor rear refinement','source':SOURCE.name,'source_sha256':hashlib.sha256(raw).hexdigest(),
            'sha256':hashlib.sha256(result).hexdigest(),'triangles':triangles,'parts':len(nodes),
            'removed_parts':removed,'wheel_streams':wheel_evidence,'preparation':'scripts/refine-cybercar.py',
            'changes':['rebuilt rear deck, louvers, wing, tail fascia, lenses and diffuser','satin body albedo and separate material families',
                       'UV atlas with baked panel seams, vent occlusion and roughness variation',
                       'small two-segment bevels and area-weighted split normals on authored rear'],
            'rear_bake':prepared['evidence'],
            'license_status':'Derived from the existing source asset; original provenance remains alongside it.'}
    OUTPUT.with_suffix('.asset.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))


if __name__ == '__main__':
    main()
