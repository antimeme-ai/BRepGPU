"""Read the actual native bridge output with the existing Fraction CDT oracle."""
from fractions import Fraction as F
from pathlib import Path
import argparse
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'python'),str(ROOT/'tests')]
from direct_cdt_oracle import qualify_cdt
from face_domain_external_cases import ExternalCase

def main(fixture):
    result=subprocess.run([str(fixture.resolve())],capture_output=True,text=True,timeout=60,check=True)
    original=((0,0),(0,8),(8,8),(8,0),(2,2),(6,2),(6,6),(2,6))
    # The literal full factor is independently applied to original coordinates,
    # not read back from a native transformed-coordinate or predicate output.
    transformed=tuple((F(x)-F(y),(F(x)+F(y))/8) for x,y in original)
    case=ExternalCase('native-domain-full-metric',((0,3,2,1),(4,7,6,5)),tuple(enumerate(transformed)))
    triangles=[]
    for line in result.stdout.splitlines():
        fields=line.split('\t')
        if fields[0]=='domain_triangle':
            if len(fields)!=4:raise AssertionError('native domain triangle schema differs')
            triangles.append(tuple(map(int,fields[1:])))
    qualify_cdt(case,triangles)
    print(result.stdout,end='')
    print('native domain: original-coordinate Fraction coverage/embedding/full-metric CDT PASS')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True)
    main(parser.parse_args().fixture)
