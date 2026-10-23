"""Closed, concentric wheel geometry in provider units (axle Z, radial XY).

All visible materials share a small authored color/metallic-roughness palette.
The source car's 4K material is reserved for the retained body.
"""
import math
import struct
import zlib

TAU=math.tau
SEGMENTS=96
COLORS=[(27,29,32),(48,51,56),(168,175,184),(63,70,80),(105,111,120),(205,211,218),(20,23,28),(135,143,154)]
ROUGH_METAL=[(.88,0),(.72,0),(.30,.85),(.42,.75),(.55,.8),(.24,.9),(.94,0),(.38,.8)]

def png(mr=False):
    width,height=256,32
    pixels=bytearray()
    for y in range(height):
        pixels.append(0)
        for x in range(width):
            slot=x//32
            if mr:
                rough,metal=ROUGH_METAL[slot];color=(255,round(rough*255),round(metal*255))
            else:color=COLORS[slot]
            pixels.extend((*color,255))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(bytes(pixels)))+chunk(b'IEND',b'')

def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):
    length=math.sqrt(sum(x*x for x in a));return tuple(x/length for x in a)

class Mesh:
    def __init__(self):self.positions=[];self.normals=[];self.uv=[];self.indices=[]
    def triangle(self,points,slot,normals=None):
        normal=cross(sub(points[1],points[0]),sub(points[2],points[0]))
        if sum(v*v for v in normal)<1e-20:return
        normal=unit(normal)
        for i,p in enumerate(points):
            self.indices.append(len(self.positions));self.positions.append(p)
            self.normals.append(normals[i] if normals else normal);self.uv.append(((slot+.5)/8,.5))
    def quad(self,points,slot,normals=None):
        for ids in [(0,1,2),(0,2,3)]:self.triangle([points[i] for i in ids],slot,[normals[i] for i in ids] if normals else None)
    def lathe(self,profile,slots,center=(0,0,0),segments=SEGMENTS,smooth=True):
        # Closed (radius, axle) profile, positive winding in this plane.
        for j,(r,z) in enumerate(profile):
            k=(j+1)%len(profile);rr,zz=profile[k]
            def normal_at(index,angle):
                before=profile[index-1];after=profile[(index+1)%len(profile)]
                if not smooth:before=(r,z);after=(rr,zz)
                dr,dz=after[0]-before[0],after[1]-before[1]
                return unit((-dz*math.cos(angle),-dz*math.sin(angle),dr))
            for i in range(segments):
                a=TAU*i/segments;b=TAU*(i+1)/segments
                points=[(r*math.cos(a),r*math.sin(a),z),(rr*math.cos(a),rr*math.sin(a),zz),
                        (rr*math.cos(b),rr*math.sin(b),zz),(r*math.cos(b),r*math.sin(b),z)]
                normals=[normal_at(j,a),normal_at(k,a),normal_at(k,b),normal_at(j,b)]
                self.quad([tuple(v+c for v,c in zip(p,center)) for p in points],slots[j%len(slots)],normals)
    def prism(self,polygon,back,front,slot):
        # CCW XY polygon, beveled face edges; separate normal islands at bevels.
        cx=sum(p[0] for p in polygon)/len(polygon);cy=sum(p[1] for p in polygon)/len(polygon)
        inset=[(cx+(x-cx)*.92,cy+(y-cy)*.92) for x,y in polygon]
        rings=[[(x,y,back) for x,y in inset],[(x,y,back+.0008) for x,y in polygon],
               [(x,y,front-.0008) for x,y in polygon],[(x,y,front) for x,y in inset]]
        for a,b in zip(rings,rings[1:]):
            for i in range(len(polygon)):
                j=(i+1)%len(polygon);self.quad([a[i],a[j],b[j],b[i]],slot)
        for i in range(1,len(polygon)-1):
            self.triangle([rings[-1][0],rings[-1][i],rings[-1][i+1]],slot)
            self.triangle([rings[0][0],rings[0][i+1],rings[0][i]],slot)

def wheel(radius):
    m=Mesh();r=radius
    # Rounded shoulders, two circumferential tread grooves, closed inner bead.
    tire=[(.059,.027),(.067,.030),(.075,.028),(.080,.023),(r,.018),(r,.010),
          (r-.0012,.009),(r-.0012,.007),(r,.006),(r,-.006),(r-.0012,-.007),
          (r-.0012,-.009),(r,-.010),(r,-.018),(.080,-.023),(.075,-.028),(.067,-.030),(.059,-.027)]
    m.lathe(tire,[0]*len(tire))
    # Closed alloy barrel with raised lips at both visible faces.
    rim=[(.052,.026),(.058,.028),(.061,.027),(.062,.024),(.061,.020),(.059,.018),
         (.059,-.018),(.061,-.020),(.062,-.024),(.061,-.027),(.058,-.028),(.052,-.026)]
    m.lathe(rim,[2,2,5,2,3,3,3,2,5,2,2,3])
    # Five thick swept spokes on each side; all pieces individually closed.
    for side in (-1,1):
        for i in range(5):
            angle=TAU*i/5
            polygon=[(.012,-.006),(.051,-.004),(.055,.004),(.032,.007),(.012,.006)]
            polygon=[(x*math.cos(angle)-y*math.sin(angle),x*math.sin(angle)+y*math.cos(angle)) for x,y in polygon]
            m.prism(polygon,.010 if side>0 else -.025,.025 if side>0 else -.010,2)
        # Solid hub caps and five lug heads. Solid rotor sits behind the spokes.
        z=.026*side
        m.lathe([(0,z+.002),(.012,z+.002),(.014,z+.001),(.014,z-.001),(.012,z-.002),(0,z-.002)],[3]*6,smooth=False)
        for i in range(5):
            angle=TAU*i/5;center=(.009*math.cos(angle),.009*math.sin(angle),.0285*side)
            m.lathe([(0,.001),(.0018,.001),(.002,0),(.002,-.001),(0,-.001)],[5]*5,center,segments=6,smooth=False)
    m.lathe([(.015,.007),(.045,.007),(.047,.005),(.047,-.005),(.045,-.007),(.015,-.007)],[4]*6)
    return m

# Source-space wheel-well aperture. Front arches have room for +/-0.42 rad steer.
# The upper arc fits +0.06 m bump; the opening continues below the rocker.
CENTERS=[(.2755,-.0643,.193),(.2755,-.0643,-.193),(-.2818,-.0643,.193),(-.2818,-.0643,-.193)]
RADII=[.0825,.0825,.083,.083]
WELL_INNER=.118
WELL_TOP_R=.090
WELL_RISE=.06/4.3
def well_planes(index):
    cx,cy,cz=CENTERS[index];rx=.098 if index<2 else .094
    # Convex half spaces n dot p <= d; open bottom to remove every old tire fragment.
    planes=[((0,0,-math.copysign(1,cz)),-WELL_INNER)]
    for i in range(33):
        angle=math.pi*i/32;nx=math.cos(angle)/rx;ny=math.sin(angle)/WELL_TOP_R
        planes.append(((nx,ny,0),1+nx*cx+ny*(cy+WELL_RISE)))
    return planes

def liner():
    m=Mesh()
    for w,(cx,cy,cz) in enumerate(CENTERS):
        sign=math.copysign(1,cz);rx=(.098 if w<2 else .094)+.0006;ry=WELL_TOP_R+.0006
        # Recessed dark arch surface, behind the wheel sweep.
        for i in range(64):
            a=math.pi*i/64;b=math.pi*(i+1)/64
            def p(t,r,z):return (cx+(rx+r)*math.cos(t),cy+WELL_RISE+(ry+r)*math.sin(t),z*sign)
            # Winding corrected for mirrored side, all surfaces retained.
            quad=[p(a,0,.116),p(a,0,.228),p(b,0,.228),p(b,0,.116)]
            if sign<0:quad.reverse()
            m.quad(quad,6)
        # Inboard enclosure is recessed past the steering envelope.
        outline=[(cx-rx,cy-.055,sign*.116),(cx+rx,cy-.055,sign*.116)]
        outline += [(cx+rx*math.cos(math.pi*i/64),cy+WELL_RISE+ry*math.sin(math.pi*i/64),sign*.116) for i in range(65)]
        if sign<0:outline.reverse()
        for i in range(1,len(outline)-1):m.triangle([outline[0],outline[i],outline[i+1]],6)
    return m
