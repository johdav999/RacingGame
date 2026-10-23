"""Bastion wheel assemblies and clearance apertures, in game metres (+Y up)."""
import math
import runpy
from pathlib import Path

shared=runpy.run_path(str(Path(__file__).with_name('cybercar-wheel-geometry.py')))
CENTERS=[(x,-.2577159916020867,z) for z in (1.3380221988044136,-1.4797283473786935)
         for x in (-.9756466657506696,.9766718612433436)]
RADIUS=.3812183022409318
# Preserve the established four physics widths, including the original fit's
# small left/right differences. Geometry is centered on those same axle pivots.
HALF_WIDTHS=[.3222799301147461/2,.3177609443664551/2,.3599197268486023/2,.3579789400100708/2]
SUSPENSION=dict(rest=.30,minimum=.24,maximum=.38)
MAX_STEER=.42
RISE=.07 # Full bump plus 10 mm of solver bump-stop overtravel.
TOP_RADIUS=.410

def well_planes(index):
    cx,cy,cz=CENTERS[index];rx=.47 if index<2 else .425
    inner=.64 if index<2 else .75
    planes=[((-math.copysign(1,cx),0,0),-inner)]
    for i in range(33):
        a=math.pi*i/32;nz=math.cos(a)/rx;ny=math.sin(a)/TOP_RADIUS
        planes.append(((0,ny,nz),1+ny*(cy+RISE)+nz*cz))
    return planes

def wheel(index):
    mesh=shared['wheel'](.083)
    # A rigid circular tire; do not apply the body's nonuniform source scaling.
    half_width=HALF_WIDTHS[index]
    def point(p):return (-p[2]*half_width/.03,p[1]*RADIUS/.083,p[0]*RADIUS/.083)
    def normal(n):return shared['unit']((-n[2]/(half_width/.03),n[1]/(RADIUS/.083),n[0]/(RADIUS/.083)))
    mesh.positions=[point(p) for p in mesh.positions];mesh.normals=[normal(n) for n in mesh.normals]
    return mesh

def liner():
    mesh=shared['Mesh']()
    for w,(cx,cy,cz) in enumerate(CENTERS):
        sign=math.copysign(1,cx);rx=(.47 if w<2 else .425)+.003;ry=TOP_RADIUS+.003
        inner=(.64 if w<2 else .75)-.012
        def point(t,x):return (x*sign,cy+RISE+ry*math.sin(t),cz+rx*math.cos(t))
        for i in range(64):
            a=math.pi*i/64;b=math.pi*(i+1)/64
            # Recess the outer edge behind the armor; a full-width strip would
            # stick out beside the narrower hood as a floating black flap.
            outer=.96 if w<2 else 1.0
            quad=[point(a,inner),point(a,outer),point(b,outer),point(b,inner)]
            if sign>0:quad.reverse()
            mesh.quad(quad,6)
        outline=[(inner*sign,cy-.25,cz-rx),(inner*sign,cy-.25,cz+rx)]
        outline += [point(math.pi*i/64,inner) for i in range(65)]
        if sign>0:outline.reverse()
        for i in range(1,len(outline)-1):mesh.triangle([outline[0],outline[i],outline[i+1]],6)
    return mesh
