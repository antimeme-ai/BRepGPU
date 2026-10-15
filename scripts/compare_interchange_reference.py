#!/usr/bin/env python3
"""Check saved foreign scalar jets against exact original source geometry."""
from array import array
import argparse
import csv
from fractions import Fraction as F
import hashlib
import math
from pathlib import Path
import struct
import sys
import time

from brepgpu.interchange import load
from brepgpu.interchange_oracles import source_jet


def tsv(path,header,rows):
    with path.open('w',newline='') as f:
        w=csv.writer(f,delimiter='\t',lineterminator='\n');w.writerow(header);w.writerows(rows)


def require(ok,message):
    if not ok:raise AssertionError(message)


def bits(x):return struct.pack('<d',x)


def analytic_jet(u,v):
    return ((u,v,F(9,4)*u*(1-u)*v*(1-v)),
            (F(1),F(0),F(9,4)*(1-2*u)*v*(1-v)),
            (F(0),F(1),F(9,4)*u*(1-u)*(1-2*v)))


def execute(args):
    require(sys.platform=='linux','qualification executes on fleet')
    started=time.perf_counter_ns()
    case,reference=args.case_directory,args.reference_directory
    conversion=load(case/'source.usda')
    recipe=dict(csv.reader((case/'recipe.tsv').open(),delimiter='\t'))
    metadata=dict(csv.reader((reference/'source.tsv').open(),delimiter='\t'))
    require(conversion.sha256==recipe['source_sha256']==metadata['sha256'],'reference/owned original bytes differ')
    domains=list(csv.DictReader((reference/'domains.tsv').open(),delimiter='\t'))
    require(len(domains)==len(conversion.faces),'reference original face inventory differs')
    require(len({face.key[1] for face in conversion.faces})==1,'reference ordinal comparison requires one prim')
    for face,row in zip(conversion.faces,domains,strict=True):
        require(int(row['face_index'])==face.key[2],'reference domain ordinal differs')
        require(face.nurbs is not None,'this reference correspondence experiment requires NURBS')
        actual=tuple(float(row[name]) for name in ('umin','umax','vmin','vmax'))
        require(all(bits(a)==bits(b) for a,b in zip(actual,face.nurbs.domain)),'reference natural domain differs from original knots')
    ideal_delta=F(0)
    if args.analytic_sheet:
        require(len(conversion.faces)==1,'analytic sheet expects one source face')
        n=conversion.faces[0].nurbs
        require((n.count_u,n.count_v,n.order_u,n.order_v)==(4,4,4,4) and n.domain==(0.,1.,0.,1.) and n.weights==(1.,)*16,'analytic sheet representation differs')
        for index,p in enumerate(n.points):
            i,j=divmod(index,4);expected=(F(i,3),F(j,3),F(1,4) if i in (1,2) and j in (1,2) else F(0))
            ideal_delta=max(ideal_delta,*(abs(F(x)-y) for x,y in zip(p,expected)))
        require(ideal_delta<=F(1,2**52),'reference sheet controls differ from intended analytic construction')
    queries=list(csv.DictReader((case/'queries.tsv').open(),delimiter='\t'))
    observed=list(csv.DictReader((reference/'reference-jets.tsv').open(),delimiter='\t'))
    count=int(recipe['queries'])
    require(len(queries)==len(observed)==count,'actual reference query coverage differs')
    require(int(metadata['queries'])==count and int(metadata['faces'])==len(conversion.faces),'reference execution metadata differs')
    metrics={key:F(value) for key,value in list(csv.reader((case/'metrics.tsv').open(),delimiter='\t'))[1:]}
    budgets=tuple(metrics['max_source_'+term+'_budget'] for term in ('position','du','dv'))
    jets=array('d');jets.frombytes((case/'source-jets.f64').read_bytes())
    require(sys.byteorder=='little' and len(jets)==9*count,'saved native array layout differs')
    by_key={face.key[1:]:face for face in conversion.faces}
    stats={key:F(0) for term in ('position','du','dv') for key in
           ('max_reference_'+term+'_error','max_reference_'+term+'_limit','max_native_reference_'+term+'_gap','max_source_query_rounding_'+term+'_effect')}
    rows=[]
    for index,(query,actual) in enumerate(zip(queries,observed,strict=True)):
        require(int(query['query'])==int(actual['query'])==index,'reference query order differs')
        key=(query['prim'],int(query['face_index']))
        require(key==(actual['prim'],int(actual['face_index'])) and key in by_key,'reference source face identity differs')
        face=by_key[key];require(face.patch_slot==int(query['slot']),'owned source patch identity differs')
        U,V=F(query['exact_u']),F(query['exact_v']);n=face.nurbs
        s,t=float.fromhex(query['s_hex']),float.fromhex(query['t_hex'])
        require(U==F(n.domain[0])+(F(n.domain[1])-F(n.domain[0]))*F(s) and
                V==F(n.domain[2])+(F(n.domain[3])-F(n.domain[2]))*F(t),'recorded exact source map differs')
        u,v=float.fromhex(actual['u_hex']),float.fromhex(actual['v_hex'])
        require(bits(u)==bits(float(U))==bits(float.fromhex(query['reference_u_hex'])) and
                bits(v)==bits(float(V))==bits(float.fromhex(query['reference_v_hex'])),'reference actual submitted UV differs')
        truth=source_jet(n,F(u),F(v));native_truth=source_jet(n,U,V)
        reference_values=tuple(float(actual[field]) for field in ('x','y','z','du_x','du_y','du_z','dv_x','dv_y','dv_z'))
        require(all(math.isfinite(value) for value in reference_values),'nonfinite reference jet')
        errors=[]
        ideal=analytic_jet(F(u),F(v)) if args.analytic_sheet else None
        for term,name in enumerate(('position','du','dv')):
            maximum=F(0)
            for axis in range(3):
                ref=F(reference_values[3*term+axis]);expected=truth[term][axis]
                limit=F(1,2**32)*max(F(1),abs(expected))
                error=abs(ref-expected)
                require(error<=limit,f'reference {key} query{index} {name} exceeds exact-source limit')
                native=F(jets[(3*term+axis)*count+index]);rounding=abs(native_truth[term][axis]-expected)
                gap=abs(native-ref)
                require(gap<=budgets[term]+rounding+limit,'native/reference mapped jet exceeds propagated source budget')
                if ideal is not None:
                    ideal_bound=ideal_delta if term==0 else 12*ideal_delta
                    require(abs(expected-ideal[term][axis])<=ideal_bound,'source sheet violates intended analytic map')
                    require(abs(ref-ideal[term][axis])<=ideal_bound+limit,'reference sheet violates intended analytic map')
                for key2,value in (('max_reference_'+name+'_error',error),('max_reference_'+name+'_limit',limit),
                                   ('max_native_reference_'+name+'_gap',gap),('max_source_query_rounding_'+name+'_effect',rounding)):
                    stats[key2]=max(stats[key2],value)
                maximum=max(maximum,error)
            errors.append(maximum)
        rows.append((index,*key,u.hex(),v.hex(),*errors))
    elapsed=(time.perf_counter_ns()-started)/1e6
    out=args.output_directory;out.mkdir(parents=True,exist_ok=False)
    tsv(out/'checks.tsv',('field','value'),(('source_sha256',conversion.sha256),('queries',count),('scalar_components',9*count),
         ('natural_domains','all original source faces exactly match observed reference domains'),('analytic_sheet',args.analytic_sheet),('analytic_coefficient_delta',ideal_delta),
         *sorted(stats.items()),('qualification_ms',elapsed)))
    tsv(out/'queries.tsv',('query','prim','face_index','u_hex','v_hex','reference_position_error','reference_du_error','reference_dv_error'),rows)
    tsv(out/'sources.tsv',('path','sha256'),((name,hashlib.sha256(Path(name).read_bytes()).hexdigest()) for name in
         ('scripts/compare_interchange_reference.py','python/brepgpu/interchange.py','python/brepgpu/usda_subset.py','python/brepgpu/interchange_oracles.py')))
    print(f'reference/source/native correspondence qualified: {count} queries / {9*count} scalar components',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case-directory',type=Path,required=True)
    p.add_argument('--reference-directory',type=Path,required=True)
    p.add_argument('--output-directory',type=Path,required=True)
    p.add_argument('--analytic-sheet',action='store_true')
    execute(p.parse_args())
