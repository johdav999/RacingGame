"""Blender: render the actual runtime GLB across spin/steering/travel extremes.

blender --background --python scripts/review-cybercar-wheels.py -- OUTPUT_DIRECTORY
Produces an HTML animation/review grid and 72 labelled-by-filename poses.
"""
import bpy,sys,math,json
from pathlib import Path
from mathutils import Vector,Matrix

root=Path(__file__).resolve().parents[1]
args=sys.argv[sys.argv.index('--')+1:]
output=Path(args[0]).resolve();output.mkdir(parents=True,exist_ok=True)
bastion=len(args)>1 and args[1]=='bastion'
asset=root/('assets/runtime/models/bastion/bastion.glb' if bastion else 'assets/runtime/models/cybercar-clean-detail/cybercar.glb')
metadata=json.loads(asset.with_suffix('.asset.json').read_text())
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(asset))
if bastion:
 for obj in list(bpy.context.scene.objects):obj.matrix_world=Matrix.Scale(1/4.6,4) @ obj.matrix_world
scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE'
scene.eevee.use_gtao=True;scene.eevee.gtao_distance=.04;scene.eevee.taa_render_samples=32
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.32,.36,.4,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
scene.view_settings.view_transform='Filmic';scene.view_settings.look='Medium High Contrast'
scene.render.resolution_x=1440;scene.render.resolution_y=960;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
center=Vector((0,0,0 if bastion else .13))
for name,offset,power,size in [('Key',(-1,-2,3),65,2),('Fill',(2,1,2),45,2),('Rim',(0,2,3),55,1.5)]:
 data=bpy.data.lights.new(name,'AREA');data.energy=power;data.size=size
 obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=center+Vector(offset)
 obj.rotation_euler=(center-obj.location).to_track_quat('-Z','Y').to_euler()
camera=bpy.data.objects.new('Wheel review camera',bpy.data.cameras.new('Wheel review camera'))
scene.collection.objects.link(camera);scene.camera=camera;camera.data.type='ORTHO';camera.data.ortho_scale=1.15
wheels=[o for o in scene.objects if o.type=='MESH' and o.name in ('wheel_front_left','wheel_front_right','wheel_rear_left','wheel_rear_right')]
original={o.name:o.matrix_world.copy() for o in wheels};poses=[]
for side,offset in [('left',(-1.5,-2,.7)),('right',(1.5,2,.7))]:
 camera.location=center+Vector(offset);camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
 for travel in [metadata['suspension'][key] for key in ['minimum','rest','maximum']]:
  for steer in [-.42,0,.42]:
   for spin in [0,math.pi/2,math.pi,3*math.pi/2]:
    for wheel in wheels:
     base=original[wheel.name];pivot=base.translation.copy()
     if bastion:
      p=next(p['pivot'] for p in metadata['parts'] if p['name']==wheel.name);pivot=Vector((p[0],-p[2],p[1]))/4.6
     shift=Vector((0,0,(metadata['suspension']['rest']-travel)/(4.6 if bastion else 4.3)))
     turn=steer if 'front' in wheel.name else 0
     wheel.matrix_world=Matrix.Translation(pivot+shift) @ Matrix.Rotation(turn,4,'Z') @ Matrix.Rotation(spin,4,'X' if bastion else 'Y') @ Matrix.Translation(-pivot) @ base
    name=f'{side}-travel-{travel:.2f}-steer-{steer:+.2f}-spin-{round(math.degrees(spin)):03}.png'
    scene.render.filepath=str(output/name);bpy.ops.render.render(write_still=True)
    poses.append(dict(file=name,side=side,suspension=travel,steering_degrees=round(math.degrees(steer),1),rotation_degrees=round(math.degrees(spin))))
record=dict(asset_sha256=json.loads(asset.with_suffix('.asset.json').read_text())['sha256'],poses=poses,
 note='Controlled geometry review of runtime GLB. Native/browser tests separately validate physics-driven presentation.')
(output/'poses.json').write_text(json.dumps(record,indent=2))
html='''<!doctype html><meta charset="utf-8"><title>Cybercar wheel motion review</title>
<style>body{background:#151b24;color:#edf4fb;font:16px system-ui;margin:24px}img{width:100%;max-width:1100px}button,input{margin:10px}#grid{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}figure{margin:0}figcaption{font-size:12px}</style>
<h1>Cybercar wheel motion review</h1><p>Actual runtime geometry: 360° rotation, ±24° steering, 0.24–0.38 m suspension length. Controlled inspection poses.</p>
<button id="play">Pause</button><input type="range" id="frame" min="0" max="71" value="0" style="width:60%"><p id="label"></p><img id="hero"><h2>All poses</h2><div id="grid"></div>
<script>const poses=POSES;let i=0,playing=true;const frame=document.querySelector('#frame'),hero=document.querySelector('#hero'),label=document.querySelector('#label');function show(){frame.value=i;hero.src=poses[i].file;label.textContent=poses[i].file}frame.oninput=()=>{i=+frame.value;playing=false;show()};document.querySelector('#play').onclick=()=>playing=!playing;setInterval(()=>{if(playing){i=(i+1)%poses.length;show()}},350);document.querySelector('#grid').innerHTML=poses.map(p=>`<figure><img loading="lazy" src="${p.file}"><figcaption>${p.file}</figcaption></figure>`).join('');show();</script>'''
if bastion:html=html.replace('Cybercar','Bastion')
(output/'index.html').write_text(html.replace('POSES',json.dumps(poses)))
print('WHEEL_REVIEW',json.dumps({'poses':len(poses),'output':str(output)}))
