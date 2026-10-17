"""Existing real boundary service with prepared/original-span admission attacks."""
from dataclasses import replace
from pathlib import Path
import argparse
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'python'))
from brepgpu.imported_source import load_source, write_source, Carrier, SurfaceSpan, ZERO_FRAME


def rational_plane(identifier, domain):
    lo,hi = domain
    controls = tuple(x for u in (lo,hi) for v in (lo,hi) for x in (u,v,0.,1.))
    return Carrier(identifier,6,ZERO_FRAME,(0.,0.),(.125,.875,.125,.875),
        (1,1),(lo,hi),(2,2),(lo,hi),(2,2),(2,2),controls,
        (SurfaceSpan((lo,hi,lo,hi),(1,1),controls),))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fixture',required=True,type=Path)
    parser.add_argument('--source',required=True,type=Path)
    args = parser.parse_args()
    source = load_source(args.source)
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)/'profile.bgsource'
        carrier = rational_plane(source.carriers[0].id,(0.,1.))
        write_source(replace(source,carriers=(carrier,*source.carriers[1:])),path)
        subprocess.run((str(args.fixture.resolve()),str(path),'--profile-regular'),check=True)
        # Natural source [-1,2]^2, requested prepared [0,1]^2. All chosen
        # endpoints are inside; the ORIGINAL cubic has midpoint v=3/2 outside.
        carrier = rational_plane(source.carriers[0].id,(-1.,2.))
        # The exact closed-boundary case must survive padded RN hulls through
        # the common source exact-hull fallback, without expanding [0,1]^2.
        write_source(replace(source,carriers=(carrier,*source.carriers[1:]),
            coedges=tuple(replace(u,reconstructed=False) for u in source.coedges)),path)
        subprocess.run((str(args.fixture.resolve()),str(path),'--profile-regular'),check=True)
        guide_id = source.coedges[0].curve_id
        inside = tuple(replace(c,kind=4,degree=3,controls=(0.,0.,1.,1./3.,.5,1.,
                       2./3.,.5,1.,1.,0.,1.)) if c.id == guide_id else c
                       for c in source.curves)
        write_source(replace(source,carriers=(carrier,*source.carriers[1:]),curves=inside,
            coedges=tuple(replace(u,reconstructed=False) for u in source.coedges)),path)
        subprocess.run((str(args.fixture.resolve()),str(path),'--profile-regular'),check=True)
        curves = tuple(replace(c,kind=4,degree=3,controls=(0.,0.,1.,1./3.,2.,1.,
                       2./3.,2.,1.,1.,0.,1.)) if c.id == guide_id else c
                       for c in source.curves)
        assert .125*0 + .375*2 + .375*2 + .125*0 == 1.5
        write_source(replace(source,carriers=(carrier,*source.carriers[1:]),curves=curves,
            coedges=tuple(replace(u,reconstructed=False) for u in source.coedges)),path)
        subprocess.run((str(args.fixture.resolve()),str(path),'--profile-auth'),check=True)
    print('PASS\tprepared rectangle, original-span and whole authoritative profile checks')


if __name__ == '__main__':
    main()
