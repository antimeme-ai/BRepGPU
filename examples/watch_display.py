"""Owned indexed-triangle CPU depth display; no geometry construction here."""
from array import array
import base64
import math
from pathlib import Path
import struct
import zlib

WIDTH,HEIGHT=1800,1500
MATERIALS={'blue':(.105,.245,.34),'silver':(.67,.74,.77),'gold':(.90,.62,.24),
           'brass':(.70,.43,.15),'steel':(.12,.17,.22),'ruby':(.63,.045,.12)}


def _unit(v):
    n=math.hypot(*v)
    if not math.isfinite(n) or n==0:raise ValueError('invalid display direction')
    return tuple(x/n for x in v)


def _camera(azimuth,elevation):
    az,el=map(math.radians,(azimuth,elevation));ca,sa,ce,se=math.cos(az),math.sin(az),math.cos(el),math.sin(el)
    return ((ca,sa,0.),(-sa*se,ca*se,ce),(sa*ce,-ca*ce,se))


def _view(p,basis):return tuple(sum(x*y for x,y in zip(p,row)) for row in basis)


def _png(pixels):
    def chunk(kind,data):return struct.pack('!I',len(data))+kind+data+struct.pack('!I',zlib.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+pixels[y*WIDTH*4:(y+1)*WIDTH*4] for y in range(HEIGHT))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',WIDTH,HEIGHT,8,6,0,0,0))
            +chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))


def render(directory,parts,*,name,azimuth,elevation,scale=410.):
    """Draw every actual indexed facet; largest interpolated view Z wins."""
    directory=Path(directory);basis=_camera(azimuth,elevation)
    light=_unit((-.35,.50,.79));half=_unit((light[0],light[1],light[2]+1))
    center=(900.,740.)
    depth=array('d',[-math.inf])*(WIDTH*HEIGHT);pixels=bytearray(WIDTH*HEIGHT*4)
    count=0
    for recipe,mesh,reader in parts:
        xyz=reader.xyz;ids=reader.indices
        if len(xyz)%3 or len(ids)%3:raise ValueError('incomplete indexed mesh')
        color=MATERIALS[recipe.material]
        points=[]
        for i in range(0,len(xyz),3):
            p=tuple(xyz[i:i+3])
            if not all(math.isfinite(x) for x in p):raise ValueError('nonfinite actual mesh point')
            x,y,z=_view(p,basis);points.append((center[0]+scale*x,center[1]-scale*y,z))
        for t in range(0,len(ids),3):
            indices=tuple(ids[t:t+3])
            if any(i>=len(points) for i in indices) or len(set(indices))!=3:raise ValueError('invalid triangle indices')
            a,b,c=(points[i] for i in indices)
            p,q,r=(tuple(xyz[3*i:3*i+3]) for i in indices)
            u=tuple(q[i]-p[i] for i in range(3));v=tuple(r[i]-p[i] for i in range(3))
            normal=_unit(_view((u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]),basis))
            diffuse=max(0.,sum(x*y for x,y in zip(normal,light)))
            specular=max(0.,sum(x*y for x,y in zip(normal,half)))**35
            # Flat geometric facets, two directional fills, no normal smoothing or fake bevel.
            fill=max(0.,-.5*normal[0]-.25*normal[1]+.30*normal[2])
            rgb=tuple(round(255*min(1.,value*(.23+.70*diffuse+.18*fill)+.35*specular)) for value in color)
            ax,ay=a[:2];bx,by=b[:2];cx,cy=c[:2]
            area=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
            count+=1
            if abs(area)<1e-12:continue
            xmin=max(0,math.ceil(min(ax,bx,cx)-.5));xmax=min(WIDTH-1,math.floor(max(ax,bx,cx)-.5))
            ymin=max(0,math.ceil(min(ay,by,cy)-.5));ymax=min(HEIGHT-1,math.floor(max(ay,by,cy)-.5))
            for y in range(ymin,ymax+1):
                py=y+.5
                for x in range(xmin,xmax+1):
                    px=x+.5
                    wa=((by-cy)*(px-cx)+(cx-bx)*(py-cy))/area
                    wb=((cy-ay)*(px-cx)+(ax-cx)*(py-cy))/area;wc=1-wa-wb
                    if min(wa,wb,wc)<-1e-10:continue
                    z=wa*a[2]+wb*b[2]+wc*c[2];index=y*WIDTH+x
                    if z<=depth[index]:continue
                    depth[index]=z;offset=4*index;pixels[offset:offset+4]=bytes((*rgb,255))
    png=_png(pixels);(directory/(name+'-surface.png')).write_bytes(png)
    encoded=base64.b64encode(png).decode('ascii')
    label='OBLIQUE / ACTUAL FACETS' if elevation<85 else 'TOP / ACTUAL FACETS'
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="1800" height="1500" viewBox="0 0 1800 1500">
<defs><radialGradient id="bg" cx="48%" cy="50%" r="65%"><stop stop-color="#1b303b"/><stop offset="1" stop-color="#070e18"/></radialGradient></defs>
<rect width="1800" height="1500" fill="url(#bg)"/>
<g fill="none" stroke="#89aab4" opacity=".11"><circle cx="900" cy="740" r="640"/><circle cx="900" cy="740" r="660"/><path d="M200 740H1600M900 70V1410"/></g>
<image width="1800" height="1500" href="data:image/png;base64,{encoded}"/>
<g font-family="sans-serif"><text x="70" y="60" fill="#8aaab6" font-size="18" letter-spacing="5">ANTIMEME-AI / BRepGPU / OWNED GEOMETRY</text>
<text x="65" y="133" fill="#f3e7d5" font-size="62" letter-spacing="3">OPENWORK / STUDY 01</text>
<text x="70" y="180" fill="#b6cbd0" font-size="24">Profile → extrusion → through-cut → mesh</text>
<text x="1730" y="62" text-anchor="end" fill="#d2a45f" font-size="17" letter-spacing="2">{label}</text>
<text x="70" y="1386" fill="#d7c6a8" font-size="22">{len(parts)} actual modeled parts / {count:,} indexed triangles / CPU construction, mesh and display</text>
<text x="70" y="1427" fill="#9bb5bf" font-size="19">Schematic, nonfunctional movement. Separate lugs and spokes; no fused gears, spring or kinematic claim.</text>
<text x="70" y="1465" fill="#718e9b" font-size="17">Original source Frames and thickness / finite bounded meshes / complete typed construction ancestry</text></g></svg>'''
    (directory/(name+'.svg')).write_text(svg+'\n')
    return (('name',name),('projection','orthographic'),('azimuth_degrees',azimuth),
            ('elevation_degrees',elevation),('width',WIDTH),('height',HEIGHT),
            ('pixels_per_source_unit',scale),('center_x',center[0]),('center_y',center[1]),
            ('actual_parts',len(parts)),('actual_triangles',count),
            ('geometry_transform','none; actual mesh rounded world coordinates'),
            ('lighting','oriented flat facets; Lambert plus directional fill and Blinn highlight'),
            ('camera_right',' '.join(map(str,basis[0]))),('camera_up',' '.join(map(str,basis[1]))),
            ('camera_toward_viewer',' '.join(map(str,basis[2]))),
            ('light_view',' '.join(map(str,light))),('ambient_gain',.23),('diffuse_gain',.70),
            ('fill_gain',.18),('fill_view','-.5 -.25 .30'),('specular_gain',.35),('specular_power',35),
            ('depth','maximum interpolated camera Z'),('pixel_rule','pixel centers; signed barycentrics; inclusive edge epsilon1e-10'),
            ('hidden_surface','CPU barycentric depth buffer'),('output','RGBA8 PNG plus SVG presentation'))
