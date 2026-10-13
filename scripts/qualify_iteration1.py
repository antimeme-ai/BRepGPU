#!/usr/bin/env python3
"""Qualify a requested backend against exact binary-input and analytic truth."""

import argparse
from array import array
import csv
from dataclasses import dataclass
from fractions import Fraction
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

from brepgpu.fixtures import (
    BILINEAR_MIDPOINT_JET, ERROR_TARGETS, TERMS, TERM_NAMES,
    analytic_jet, exact_component_error, exact_jet,
    qualification_fixtures, qualification_parameters,
)


@dataclass
class Witness:
    fixture: object
    query_index: int
    order: int
    s: float
    t: float
    term: int
    axis: int
    observed: float
    expected: Fraction
    error: object
    oracle: str
    status: int


def write_table(path, columns, rows):
    with path.open("w", newline="") as stream:
        writer=csv.writer(stream,delimiter="\t")
        writer.writerow(columns)
        writer.writerows(rows)


def vector(jets, term, request):
    return tuple(jets.data[(term*3+axis)*jets.count+request] for axis in range(3))


def write_witness(directory, prefix, witness):
    f=witness.fixture
    write_table(directory/(prefix+"-controls.tsv"),
                ("control","Ax_hex","Ay_hex","Az_hex","W_hex"),
                [(i,*(float(x).hex() for x in f.controls[4*i:4*i+4])) for i in range(f.control_count)])
    write_table(directory/(prefix+"-query.tsv"),
                ("fixture","p","q","query_index","order","s_hex","t_hex","term","axis","observed_hex",
                 "expected_numerator_hex","expected_denominator_hex","error","oracle","status"),
                [(f.name,f.degree_u,f.degree_v,witness.query_index,witness.order,
                  witness.s.hex(),witness.t.hex(),TERM_NAMES[witness.term],witness.axis,
                  witness.observed.hex(),format(witness.expected.numerator,"x"),
                  format(witness.expected.denominator,"x"),
                  repr(float(witness.error)) if witness.error is not None else "nonfinite",witness.oracle,witness.status)])


def qualify(backend,device,directory,library_path=None,plugin_path=None):
    from brepgpu import Engine,Patch
    fixtures=qualification_fixtures()
    parameters=qualification_parameters()
    requests=[(slot,s,t) for slot in range(len(fixtures)) for s,t in parameters]
    requests=requests[::2]+list(reversed(requests[1::2]))
    maxima=[Fraction(0)]*3
    worst=[None]*3
    failures=[]
    rows=[]
    checks=0
    evaluated=0
    with Engine(backend=backend,device=device,library_path=library_path,plugin_path=plugin_path) as engine:
        patches=[Patch(f.degree_u,f.degree_v,f.controls) for f in fixtures]
        with engine.create_patch_set(patches) as patch_set:
            slots=array("I",(slot for slot,s,t in requests))
            uv=array("d",(x for slot,s,t in requests for x in (s,t)))
            with engine.create_query_set(patch_set,patch_slots=slots,uv=uv) as query_set:
                for order in (0,1,2):
                    with engine.evaluate(query_set,order=order) as result:
                        jets=result.download()
                    if jets.count!=len(requests) or jets.order!=order:
                        raise AssertionError("result count/order differs from the requested batch")
                    evaluated+=jets.count
                    for index,(slot,s,t) in enumerate(requests):
                        f=fixtures[slot]
                        query_status=int(jets.statuses[index])
                        truths=[("binary-input",exact_jet(f,s,t,order))]
                        if f.analytic_kind:
                            truths.append(("analytic",analytic_jet(f,s,t,order)))
                        if f.analytic_kind=="bilinear" and s==0.5 and t==0.5:
                            truths.append(("literal-dyadic",BILINEAR_MIDPOINT_JET[:(1,3,6)[order]]))
                        for oracle,truth in truths:
                            for term,expected in enumerate(truth):
                                grade=sum(TERMS[term])
                                for axis,exact in enumerate(expected):
                                    observed=vector(jets,term,index)[axis]
                                    error=exact_component_error(observed,exact)
                                    checks+=1
                                    witness=Witness(f,index,order,s,t,term,axis,observed,exact,error,oracle,query_status)
                                    if query_status or error is None or error>ERROR_TARGETS[grade] or (oracle=="literal-dyadic" and error):
                                        failures.append(witness)
                                    if error is not None and (worst[grade] is None or error>maxima[grade]):
                                        maxima[grade]=error
                                        worst[grade]=witness
                        rows.append((backend,device,order,index,f.name,slot,s.hex(),t.hex(),int(jets.statuses[index])))
    directory.mkdir(parents=True,exist_ok=True)
    write_table(directory/"qualification-requests.tsv",
                ("backend","device","order","request","fixture","patch_slot","s_hex","t_hex","status"),rows)
    write_table(directory/"qualification-summary.tsv",
                ("grade","max_component_error","target","requests","component_checks","failure_components","backend","device"),
                [(grade,repr(float(maxima[grade])),repr(float(ERROR_TARGETS[grade])),evaluated,checks,len(failures),backend,device)
                 for grade in range(3)])
    for grade,witness in enumerate(worst):
        if witness:write_witness(directory,f"worst-grade-{grade}",witness)
    for i,witness in enumerate(failures):write_witness(directory,f"failure-{i:04d}",witness)
    report=(f"# Iteration-one arithmetic qualification\n\n"
            f"Backend: `{backend}`; device ordinal: `{device}`.\n\n"
            f"{evaluated} valid requests; {checks} direct component checks; {len(failures)} failed components. "
            f"All sixteen degree pairs and all requested orders were evaluated. "
            f"Truth uses exact ratios of supplied binary64 inputs, plus independent analytic/literal fixtures.\n\n"
            f"| Grade | Maximum absolute component error | Target |\n| --- | ---: | ---: |\n"
            +"".join(f"| {i} | {float(maxima[i]):.17g} | {float(ERROR_TARGETS[i]):.17g} |\n" for i in range(3))
            +"\nHex controls/queries and exact expectation ratios accompany each worst-grade and failed component. "
            "This qualifies the finite corpus; it is not a per-result enclosure, trim validity, or modeling speedup. "
            "Caller/ABI/lifetime/frame conformance is checked separately by the runtime test unit.\n")
    (directory/"QUALIFICATION.md").write_text(report)
    return not failures


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend",choices=("cpu","cuda"),required=True)
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--output-directory",type=Path,required=True)
    parser.add_argument("--library-path")
    parser.add_argument("--plugin-path")
    args=parser.parse_args()
    try:
        success=qualify(args.backend,args.device,args.output_directory,args.library_path,args.plugin_path)
    except Exception as error:
        # A requested backend that cannot execute is a failed qualification.
        # Keep operational path details on stderr/private logs, not public reports.
        print(f"Qualification failed: {type(error).__name__}: {error}",file=sys.stderr)
        return 1
    print(f"Qualification {'passed' if success else 'FAILED'}: {args.backend} device {args.device}; "
          f"report {args.output_directory/'QUALIFICATION.md'}")
    return 0 if success else 1


if __name__=="__main__":
    raise SystemExit(main())
