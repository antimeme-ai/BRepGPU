"""Owned fixed-camera display of qualified native NCSX surface samples.

Geometry construction and numerical/source qualification belong to the caller.
Points are already in source/world coordinates; normals are per patch sample.
"""
from array import array
import base64
import csv
from html import escape
import math
from pathlib import Path
import struct
import zlib


WIDTH, HEIGHT = 1800, 1200
AZIMUTH, ELEVATION = 35., 25.
PIXELS_PER_SOURCE_UNIT = 270.
SCREEN_CENTER = (900., 650.)
PALETTE = ((.87,.46,.23), (.97,.64,.34), (.72,.32,.16), (.10,.48,.51))
LIGHT_VIEW = (-.38,.48,.79)


def _unit(vector):
    length = math.hypot(*vector)
    if not math.isfinite(length) or length == 0:
        raise ValueError('unusable display normal or light')
    return tuple(x/length for x in vector)


def _camera():
    az, el = math.radians(AZIMUTH), math.radians(ELEVATION)
    ca, sa, ce, se = math.cos(az), math.sin(az), math.cos(el), math.sin(el)
    # right x up = toward-viewer; source Z projects upward.
    return ((-sa,ca,0.), (-se*ca,-se*sa,ce), (ce*ca,ce*sa,se))


def _view(vector, basis):
    return tuple(sum(a*b for a,b in zip(row,vector)) for row in basis)


def _triples(values, label):
    result = []
    for value in values:
        row = tuple(float(x) for x in value)
        if len(row) != 3 or not all(math.isfinite(x) for x in row):
            raise ValueError(label+' must contain finite XYZ triples')
        result.append(row)
    return result


def _png(pixels):
    def chunk(kind, payload):
        return (struct.pack('!I',len(payload))+kind+payload
                +struct.pack('!I',zlib.crc32(kind+payload)&0xffffffff))
    raw = b''.join(b'\0'+pixels[y*WIDTH*4:(y+1)*WIDTH*4] for y in range(HEIGHT))
    return (b'\x89PNG\r\n\x1a\n'
            +chunk(b'IHDR',struct.pack('!2I5B',WIDTH,HEIGHT,8,6,0,0,0))
            +chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))


def _surface(points, normals, tiles):
    basis = _camera()
    light = _unit(LIGHT_VIEW)
    projected = [None]*len(points)
    for start,nu,nv,material in tiles:
        color = PALETTE[material]
        for index in range(start,start+nu*nv):
            x,y,z = _view(points[index],basis)
            normal = _unit(_view(normals[index],basis))
            # Two-sided light is intentional: this is a surface display.
            diffuse = abs(sum(a*b for a,b in zip(normal,light)))
            facing = abs(normal[2])
            specular = facing**28
            rim = (1-facing)**3
            rgb = tuple(min(255.,255*(c*(.24+.72*diffuse)+.26*specular+.07*rim)) for c in color)
            projected[index] = (SCREEN_CENTER[0]+PIXELS_PER_SOURCE_UNIT*x,
                                SCREEN_CENTER[1]-PIXELS_PER_SOURCE_UNIT*y,z,*rgb)
    depth = array('d',[-math.inf])*(WIDTH*HEIGHT)
    pixels = bytearray(WIDTH*HEIGHT*4)

    def triangle(ia,ib,ic):
        a,b,c = projected[ia],projected[ib],projected[ic]
        ax,ay=a[:2]; bx,by=b[:2]; cx,cy=c[:2]
        area=(by-cy)*(ax-cx)+(cx-bx)*(ay-cy)
        if abs(area)<1e-12:
            return
        xmin=max(0,math.ceil(min(ax,bx,cx)-.5)); xmax=min(WIDTH-1,math.floor(max(ax,bx,cx)-.5))
        ymin=max(0,math.ceil(min(ay,by,cy)-.5)); ymax=min(HEIGHT-1,math.floor(max(ay,by,cy)-.5))
        for y in range(ymin,ymax+1):
            py=y+.5
            for x in range(xmin,xmax+1):
                px=x+.5
                wa=((by-cy)*(px-cx)+(cx-bx)*(py-cy))/area
                wb=((cy-ay)*(px-cx)+(ax-cx)*(py-cy))/area
                wc=1-wa-wb
                if min(wa,wb,wc)<-1e-10:
                    continue
                z=wa*a[2]+wb*b[2]+wc*c[2]
                index=y*WIDTH+x
                if z<=depth[index]:
                    continue
                depth[index]=z
                offset=4*index
                for channel in range(3):
                    pixels[offset+channel]=max(0,min(255,round(
                        wa*a[channel+3]+wb*b[channel+3]+wc*c[channel+3])))
                pixels[offset+3]=255

    for start,nu,nv,_ in tiles:
        for u in range(nu-1):
            for v in range(nv-1):
                a=start+u*nv+v; b=a+nv; c=b+1; d=a+1
                triangle(a,b,c)
                triangle(a,c,d)
    return _png(pixels)


def _scene(path, tiles, summary):
    basis = _camera()
    rows = [('projection','orthographic'), ('width',WIDTH), ('height',HEIGHT),
            ('azimuth_degrees',AZIMUTH), ('elevation_degrees',ELEVATION),
            ('source_up_axis','Z'), ('source_camera_target','0 0 0'),
            ('pixels_per_source_unit',PIXELS_PER_SOURCE_UNIT),
            ('screen_center_x',SCREEN_CENTER[0]), ('screen_center_y',SCREEN_CENTER[1]),
            ('position_input','already source/world; renderer applies no source scale'),
            ('native_normalization_scale',4), ('native_normalization_origin','0 0 0'),
            ('normal_input','unit source/world native first-jet normals, per patch sample'),
            ('lighting','two-sided absolute Lambert plus view-facing highlight and rim'),
            ('light_view_x',LIGHT_VIEW[0]), ('light_view_y',LIGHT_VIEW[1]), ('light_view_z',LIGHT_VIEW[2]),
            ('diffuse_ambient',.24), ('diffuse_gain',.72), ('highlight_gain',.26),
            ('highlight_power',28), ('rim_gain',.07), ('rim_power',3),
            ('sample_order','tile contiguous; u-major / v-fast'),
            ('quad_split','(u,v),(u+1,v),(u+1,v+1); (u,v),(u+1,v+1),(u,v+1)'),
            ('depth_rule','largest interpolated camera Z wins; opaque surface pixels'),
            ('pixel_rule','pixel centers; signed barycentrics; inclusive edge epsilon1e-10'),
            ('png','RGBA8, transparent background, filter0, noninterlaced'),
            ('source_fit','sampled source diagnostics are separate; no global chord bound'),
            ('pack_section','illustrative rectangular sections; not as-built dimensions'),
            ('plasma','source-derived fitted plasma boundary; not vacuum vessel'),
            ('patches',summary['patches']), ('samples',summary['samples']),
            ('backend',summary['backend']), ('device',summary['device']),
            ('display_triangles',sum(2*(nu-1)*(nv-1) for _,nu,nv,_ in tiles))]
    for name,row in zip(('camera_right','camera_up','camera_toward_viewer'),basis):
        rows.extend((name+'_'+axis,value) for axis,value in zip(('x','y','z'),row))
    for material,color in enumerate(PALETTE):
        rows.extend((f'material_{material}_'+axis,value) for axis,value in zip(('r','g','b'),color))
        rows.append((f'material_{material}_role','plasma' if material==3 else 'source coil family '+str(material)))
    with path.open('w',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n')
        writer.writerow(('field','value'))
        writer.writerows(rows)


def render(path, points, normals, tiles, summary):
    """Write fixed-camera SVG, transparent surface.png and explicit scene.tsv."""
    path = Path(path)
    points = _triples(points,'positions')
    normals = _triples(normals,'normals')
    if len(points)!=len(normals) or type(summary['samples']) is not int or summary['samples']!=len(points):
        raise ValueError('display sample extent differs from actual native arrays')
    tiles = tuple(tuple(tile) for tile in tiles)
    cursor = 0
    for tile in tiles:
        if len(tile)!=4 or any(type(x) is not int for x in tile):
            raise TypeError('tile must contain integer start/nu/nv/material')
        start,nu,nv,material = tile
        if start!=cursor or nu<2 or nv<2 or not 0<=material<len(PALETTE):
            raise ValueError('display tiles must cover the contiguous sample arrays in order')
        cursor += nu*nv
    if cursor!=len(points) or type(summary['patches']) is not int or summary['patches']!=len(tiles):
        raise ValueError('display patch layout does not cover all samples')
    if any(abs(math.hypot(*normal)-1)>1e-6 for normal in normals):
        raise ValueError('display normals must be checked native unit normals')
    png = _surface(points,normals,tiles)
    encoded = base64.b64encode(png).decode('ascii')
    backend = str(summary['backend'])
    device = str(summary['device'])
    current = ('P100 surface study 00 / CUDA device '+device if backend=='cuda'
               else 'native CPU surface study 00')
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1800" height="1200" viewBox="0 0 1800 1200">',
             '<defs><radialGradient id="sky" cx="50%" cy="46%" r="68%"><stop stop-color="#19303a"/><stop offset="1" stop-color="#07111b"/></radialGradient></defs>',
             '<rect width="1800" height="1200" fill="url(#sky)"/>',
             '<g fill="none" stroke="#779ca8" opacity=".10">',
             '<path d="M70 650H1730M900 210V1055"/>',
             '<ellipse cx="900" cy="650" rx="690" ry="300"/>',
             '<ellipse cx="900" cy="650" rx="510" ry="220"/></g>',
             f'<image x="0" y="0" width="1800" height="1200" href="data:image/png;base64,{encoded}"/>',
             '<g font-family="sans-serif">',
             '<text x="60" y="53" fill="#87a6b3" font-size="16" letter-spacing="4">ANTIMEME-AI / BRepGPU / SOURCE GEOMETRY STUDIES</text>',
             '<text x="56" y="116" fill="#f4e7d5" font-size="49" letter-spacing="2">NCSX / GOAL OBJECT</text>',
             '<text x="60" y="156" fill="#b2c8ce" font-size="21">18 modular coils / 3 periods / source-derived plasma boundary</text>',
             f'<text x="60" y="192" fill="#d6a476" font-size="18">Current BRepGPU: {escape(current)}</text>',
             '<text x="60" y="1100" fill="#c8d8dd" font-size="19">Illustrative coil-pack sections / fitted plasma boundary / native derivative lighting</text>',
             '<text x="60" y="1133" fill="#91acb8" font-size="17">Surface assembly, not as-built CAD. Plasma is not the vacuum vessel. No global fit or mesh error bound.</text>',
             f'<text x="60" y="1172" fill="#7c9ca9" font-size="16">{summary["patches"]:,} patches / {summary["samples"]:,} native first-jet samples / fixed camera 35° azimuth, 25° elevation</text>',
             '</g></svg>']
    path.parent.mkdir(parents=True,exist_ok=True)
    path.with_name('surface.png').write_bytes(png)
    _scene(path.with_name('scene.tsv'),tiles,summary)
    path.write_text('\n'.join(parts)+'\n')
    return path
