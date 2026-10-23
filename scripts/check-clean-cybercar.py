"""Validate closed wheels, retained body textures and the full motion envelope."""
from collections import Counter,defaultdict
from pathlib import Path
import hashlib,json,math,runpy
m=runpy.run_path(str(Path(__file__).with_name('prepare-clean-cybercar.py')))
g=runpy.run_path(str(Path(__file__).with_name('cybercar-wheel-geometry.py')))
raw,d,b=m['load'](m['DEST']/'cybercar.glb');_,source,source_bin=m['load'](m['SOURCE'])
asset=json.loads((m['DEST']/'cybercar.asset.json').read_text());read=m['read'];report={'parts':[]}
assert [n['name'] for n in d['nodes']]==['cybercar_chassis',*m['WHEELS'],'cybercar_wheel_wells']
low=min(v[1] for v in read(source,source_bin,source['meshes'][0]['primitives'][0]['attributes']['POSITION']))
for node in d['nodes']:
 p=d['meshes'][node['mesh']]['primitives'][0];vs=read(d,b,p['attributes']['POSITION']);ix=[v[0] for v in read(d,b,p['indices'])]
 if node['name'].startswith('wheel_'):
  keys=[tuple(round(x,7) for x in v) for v in vs];edges=Counter();directions=Counter();neighbors=defaultdict(set)
  for i in range(0,len(ix),3):
   a,c,e=[keys[j] for j in ix[i:i+3]]
   assert len({a,c,e})==3,'Degenerate wheel triangle'
   for u,v in [(a,c),(c,e),(e,a)]:
    edges[tuple(sorted((u,v)))]+=1;directions[(u,v)]+=1;neighbors[u].add(v);neighbors[v].add(u)
  assert all(count==2 for count in edges.values()),(node['name'],'open or nonmanifold edges',Counter(edges.values()))
  assert all(directions[(v,u)]==count for (u,v),count in directions.items()),'Inconsistent winding'
  unseen=set(neighbors);components=0
  while unseen:
   stack=[unseen.pop()];components+=1
   while stack:
    for v in neighbors[stack.pop()]:
     if v in unseen:unseen.remove(v);stack.append(v)
  assert components==25,(node['name'],components)
  radius=node['extras']['radius_source_units'];assert max(math.hypot(v[0],v[1]) for v in vs)<=radius+1e-7
  assert max(abs(v[2]) for v in vs)<=.0300001
  report['parts'].append(dict(name=node['name'],triangles=len(ix)//3,closed_components=components,boundary_edges=0))
 elif node['name'] in ('cybercar_chassis','cybercar_wheel_wells'):
  original=[(-v[0],v[1]+low,-v[2]) for v in vs]
  # Every retained triangle lies entirely outside at least one plane of each well.
  for w in range(4):
   planes=g['well_planes'](w)
   for i in range(0,len(ix),3):
    tri=[original[j] for j in ix[i:i+3]]
    assert any(all(sum(a*x for a,x in zip(n,v))-offset>=-2e-6 for v in tri) for n,offset in planes),'Body fragment remains inside wheel well'

# An enclosing cylinder bounds every wheel vertex at EVERY spin angle. Test the
# steering continuum in .01-radian steps and subtract its Lipschitz error bound.
# Suspension is linear, so its two endpoints bound every intermediate length.
clearances=[]
for w,c in enumerate(g['CENTERS']):
 clearance=1e9;r=g['RADII'][w]
 for i in range(85):
  a=-.42+i*.01 if w<2 else 0
  for travel in [asset['suspension']['minimum']-.01,asset['suspension']['maximum']]:
   center=(c[0],c[1]+(.30-travel)/4.3,c[2])
   for n,offset in g['well_planes'](w):
    nx,ny,nz=n;x=nx*math.cos(a)-nz*math.sin(a);z=nx*math.sin(a)+nz*math.cos(a)
    support=r*math.hypot(x,ny)+.030*abs(z)
    gap=(offset-sum(a*x for a,x in zip(n,center))-support)/math.sqrt(sum(v*v for v in n))*4.3
    clearance=min(clearance,gap)
 clearance-=math.hypot(r,.03)*.005*4.3 if w<2 else 0
 assert clearance>.008,('Wheel travel/steering clearance including 10 mm bump-stop overtravel',w,clearance)
 clearances.append(clearance)
def image_bytes(doc,binary,image):
 view=doc['bufferViews'][image['bufferView']];start=view.get('byteOffset',0)
 return binary[start:start+view['byteLength']]
for a,c in zip(source['images'],d['images']):assert image_bytes(source,source_bin,a)==image_bytes(d,b,c)
assert len(d['images'])==5 and len(d['materials'])==2
assert hashlib.sha256(raw).hexdigest()==asset['sha256']
report.update(sha256=asset['sha256'],minimum_clearance_metres=clearances,rotation='entire 360 degrees (cylinder bound)',steering_radians=[-.42,.42],suspension_metres=[.24,.38],bump_stop_overtravel_allowance=.01,source_body_images='byte-identical',old_wheel_geometry='removed from all four apertures')
print(json.dumps(report,indent=2))
