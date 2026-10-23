"""Check wheel closure and clearance throughout spin, steer and suspension travel."""
from collections import Counter,defaultdict
from pathlib import Path
import hashlib,io,json,math,runpy
from PIL import Image

root=Path(__file__).resolve().parents[1]
helper=runpy.run_path(str(root/'scripts/prepare-clean-cybercar.py'))
geo=runpy.run_path(str(root/'scripts/bastion-wheel-geometry.py'))
raw,doc,binary=helper['load'](root/'assets/runtime/models/bastion/bastion.glb')
asset=json.loads((root/'assets/runtime/models/bastion/bastion.asset.json').read_text())
assert [n['name'] for n in doc['nodes']]==['chassis',*helper['WHEELS'],'wheel_wells']
report=dict(parts=[])
for node,part in zip(doc['nodes'],asset['parts']):
    primitive=doc['meshes'][node['mesh']]['primitives'][0]
    vertices=helper['read'](doc,binary,primitive['attributes']['POSITION'])
    indices=[i[0] for i in helper['read'](doc,binary,primitive['indices'])]
    if node['name'] in helper['WHEELS']:
        vertices=[tuple(a-b for a,b in zip(v,part['pivot'])) for v in vertices]
        keys=[tuple(round(x,6) for x in v) for v in vertices]
        edges=Counter();directions=Counter();neighbors=defaultdict(set)
        for i in range(0,len(indices),3):
            a,b,c=[keys[j] for j in indices[i:i+3]]
            assert len({a,b,c})==3,'Degenerate wheel triangle'
            for u,v in [(a,b),(b,c),(c,a)]:
                edges[tuple(sorted((u,v)))]+=1;directions[(u,v)]+=1;neighbors[u].add(v);neighbors[v].add(u)
        assert all(n==2 for n in edges.values()),(node['name'],Counter(edges.values()))
        assert all(directions[(v,u)]==n for (u,v),n in directions.items()),'Inconsistent winding'
        unseen=set(neighbors);components=0
        while unseen:
            stack=[unseen.pop()];components+=1
            while stack:
                for v in neighbors[stack.pop()]:
                    if v in unseen:unseen.remove(v);stack.append(v)
        assert components==25
        assert max(math.hypot(v[1],v[2]) for v in vertices)<=geo['RADIUS']+1e-6
        assert max(abs(v[0]) for v in vertices)<=geo['HALF_WIDTHS'][helper['WHEELS'].index(node['name'])]+1e-6
        report['parts'].append(dict(name=node['name'],triangles=len(indices)//3,closed_components=components,boundary_edges=0))
    else:
        for w in range(4):
            planes=geo['well_planes'](w)
            for i in range(0,len(indices),3):
                tri=[vertices[j] for j in indices[i:i+3]]
                assert any(all(sum(a*b for a,b in zip(n,v))-d>=-2e-6 for v in tri) for n,d in planes),'Static fragment inside wheel aperture'

# Cylinder support covers EVERY wheel rotation. Subtract the Lipschitz bound
# between steering samples to cover every intermediate angle, too. Suspension
# is linear, so its endpoints bound every intermediate length.
clearances=[]
for w,c in enumerate(geo['CENTERS']):
    clearance=1e9;r=geo['RADIUS'];h=geo['HALF_WIDTHS'][w]
    for i in range(85):
        angle=-.42+i*.01 if w<2 else 0
        for travel in [asset['suspension']['minimum']-.01,asset['suspension']['maximum']]:
            center=(c[0],c[1]+asset['suspension']['rest']-travel,c[2])
            for n,d in geo['well_planes'](w):
                nx,ny,nz=n;x=nx*math.cos(angle)-nz*math.sin(angle);z=nx*math.sin(angle)+nz*math.cos(angle)
                support=r*math.hypot(z,ny)+h*abs(x)
                clearance=min(clearance,(d-sum(a*b for a,b in zip(n,center))-support)/math.sqrt(sum(v*v for v in n)))
    clearance-=math.hypot(r,h)*.005 if w<2 else 0
    assert clearance>.008,(w,clearance)
    clearances.append(clearance)
def image_bytes(d,b,image):
    view=d['bufferViews'][image['bufferView']];start=view.get('byteOffset',0)
    return b[start:start+view['byteLength']]
_,source,source_bin=helper['load'](root/'artifacts/model-jobs/bastion/model.glb')
for a,b in zip(source['images'],doc['images']):
    original=Image.open(io.BytesIO(image_bytes(source,source_bin,a))).resize((1024,1024),Image.Resampling.LANCZOS)
    actual=Image.open(io.BytesIO(image_bytes(doc,binary,b)))
    assert original.tobytes()==actual.tobytes(),'Retained body maps changed'
assert hashlib.sha256(raw).hexdigest()==asset['sha256']
report.update(sha256=asset['sha256'],minimum_clearance_metres=clearances,rotation='entire 360 degrees (cylinder bound)',
    steering_radians=[-.42,.42],suspension_metres=[asset['suspension']['minimum'],asset['suspension']['maximum']],bump_stop_overtravel_allowance=.01,
    body_maps='original 1024px preparation preserved',old_wheel_geometry='removed from all four apertures')
print(json.dumps(report,indent=2))
