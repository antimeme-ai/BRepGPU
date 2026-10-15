"""Owned source recipes for a schematic, nonfunctional openwork movement.

All positions are actual source Frames; there is no cosmetic geometry transform.
Tooth-like lugs and spokes are separate extruded assembly pieces, not fused gears.
"""
from dataclasses import dataclass
from fractions import Fraction as F
import math

from brepgpu.body import Frame, make_body
from brepgpu.extrusion import make_profile, extrude
from brepgpu.feature_bore import difference


@dataclass(frozen=True)
class Recipe:
    name: str
    role: str
    material: str
    points: tuple
    origin: tuple
    scale: float = 1.
    height: F = F(1, 4)
    cutter_scale: float | None = None
    cutter_offset: tuple = (0., 0.)


def polygon(radius, count=24, phase=0.):
    """The rounded lattice polygon itself is the original modeled source."""
    return tuple((F(round(256*radius*math.cos(phase+2*math.pi*i/count)),256),
                  F(round(256*radius*math.sin(phase+2*math.pi*i/count)),256))
                 for i in range(count))


def bar(half_length, half_width, angle=0.):
    ca,sa=math.cos(angle),math.sin(angle)
    return tuple((F(round(256*(ca*x-sa*y)),256),
                  F(round(256*(sa*x+ca*y)),256))
                 for x,y in ((-half_length,-half_width),(half_length,-half_width),
                             (half_length,half_width),(-half_length,half_width)))


def recipes():
    parts=[]
    def add(name,role,material,points,origin,scale=1.,height=F(1,4),cutter=None,offset=(0.,0.)):
        parts.append(Recipe(name,role,material,points,tuple(origin),scale,height,cutter,offset))
    # Deep rim and floating asymmetrical bridges establish a visible open chassis.
    add('chassis','pierced structural rim','blue',polygon(.71875,32),(0.,0.,-.62),2.,cutter=4.)
    # Actual off-center cuts make the operation visible in non-circular outlines.
    bridge=tuple((F(x,256),F(y,256)) for x,y in
                 ((-178,-53),(134,-53),(178,-15),(142,53),(-130,53),(-178,15)))
    add('upper-bridge','off-center pierced upper support','silver',bridge,
        (-.10,.87,.43),1.,cutter=.5,offset=(.24,0.))
    add('lower-bridge','off-center pierced lower support','blue',bridge,
        (.18,-.94,.32),1.,cutter=.5,offset=(-.28,0.))
    wheels=(('barrel',(-.56,.40,.02),.43,32,.5,'gold'),
            ('center-wheel',(.19,.48,.10),.335,28,.5,'brass'),
            ('third-wheel',(.57,-.055,.19),.265,24,.5,'gold'),
            ('escape-study',(.27,-.52,.28),.235,20,.5,'brass'))
    for name,center,radius,teeth,cutter,material in wheels:
        add(name,'pierced schematic wheel blank',material,polygon(radius,24),center,cutter=cutter)
        # Separate actual lugs: nonfunctional rectangular tooth forms, inventoried.
        for i in range(teeth):
            a=2*math.pi*(i+.25)/teeth
            reach=radius+.027
            origin=(center[0]+reach*math.cos(a),center[1]+reach*math.sin(a),center[2])
            add(f'{name}-lug-{i:02d}','separate schematic radial lug',material,
                bar(.040,.014,a),origin)
        # A dark center post above the open bore: a separate part, not a filled mesh hole.
        add(name+'-arbor','separate arbor-like post','steel',polygon(.045,12),
            (center[0],center[1],center[2]+.13),.5,height=F(3,4))
    # Large open balance-like ring; separate spokes intentionally remain assembly pieces.
    center=(-.49,-.44,.38)
    add('balance-rim','pierced balance-like ring','gold',polygon(.46875,32),center,cutter=1.)
    for i in range(3):
        a=math.pi/6+2*math.pi*i/3
        origin=(center[0]+.14*math.cos(a),center[1]+.14*math.sin(a),center[2]+.02)
        add(f'balance-spoke-{i}','separate balance-like spoke','brass',bar(.16,.018,a),origin)
    add('balance-hub','pierced balance-like hub','silver',polygon(.135,16),
        (center[0],center[1],center[2]+.09),cutter=.5)
    # Ruby-like collars retain visible holes; small solid pins sit below them.
    bearings=((-.56,.40,.37),(.19,.48,.45),(.57,-.055,.54),(.27,-.52,.63),
              (-.49,-.44,.78),(.14,.87,.70),(-.10,-.94,.55))
    for i,position in enumerate(bearings):
        add(f'bearing-{i}','pierced ruby-colored bearing study','ruby',polygon(.15625,16),
            position,.5,cutter=.25)
        add(f'bearing-seat-{i}','separate metallic bearing seat','silver',polygon(.19,16),
            (position[0],position[1],position[2]-.125),.5,cutter=.25)
    # Repeated bright fasteners and dark radial indexes sharpen the outer silhouette.
    for i in range(12):
        a=2*math.pi*i/12
        xy=(1.23*math.cos(a),1.23*math.sin(a))
        add(f'fastener-{i:02d}','schematic solid fastener head','silver',polygon(.105,8),
            (*xy,-.24),.5)
        add(f'fastener-slot-{i:02d}','separate raised head stripe, not machined slot','steel',
            bar(.065,.012,a+math.pi/4),(*xy,-.165),.5)
    return tuple(parts)


def construct(recipe):
    profile=make_profile(recipe.points,Frame(recipe.origin,recipe.scale))
    feature=extrude(profile,recipe.height)
    if recipe.cutter_scale is None:
        return profile,feature,None
    dx,dy=recipe.cutter_offset
    origin=(recipe.origin[0]+dx,recipe.origin[1]+dy,recipe.origin[2])
    cutter=make_body('cylinder',Frame(origin,recipe.cutter_scale))
    return profile,feature,difference(feature,cutter)
