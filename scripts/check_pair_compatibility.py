#!/usr/bin/env python3
"""Exercise actual old artifacts across the additive ABI4 boundary, on the fleet."""
import argparse
import csv
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    spec = importlib.util.spec_from_file_location("compat_adapter",path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def worker(args):
    module = load(args.adapter)
    patch = module.Patch(1,1,((-1.,-1.,0.,1.),(-1.,1.,0.,1.),
                              (1.,-1.,0.,1.),(1.,1.,0.,1.)))
    with module.Engine(backend=args.backend,device=args.device,library_path=args.core,
                       plugin_path=args.plugin) as engine:
        with engine.create_patch_set((patch,)) as patches:
            with engine.create_query_set(patches,(0,),(.25,.75)) as queries:
                expected = (-.5,.5,0., 2.,0.,0., 0.,2.,0., 0.,0.,0., 0.,0.,0., 0.,0.,0.)
                for iteration in range(2):
                    with engine.evaluate(queries,2) as result:
                        batch = result.download()
                        if tuple(batch.data)!=expected or tuple(batch.statuses)!=(0,):
                            raise AssertionError("actual legacy evaluator differs from analytic bilinear jets")
                    if iteration==0 and args.expect!="absent":
                        try:
                            with engine.create_pair_index(((0.,)*6,)) as index:
                                if args.expect!="ok":
                                    raise AssertionError("legacy artifact unexpectedly accepted pairs")
                                with engine.discover_pairs(index,index) as page:
                                    if tuple(page.download().words)!=(1,):
                                        raise AssertionError("current pair request differs from closed point truth")
                        except module.BgError as error:
                            if args.expect!="unsupported" or error.code!=module.BG_UNSUPPORTED:
                                raise
    print("2\t36\t"+args.expect)


def controller(args):
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():
        raise FileExistsError(args.output)
    private = ROOT/"context"/"pair"/"compatibility"
    private.mkdir(parents=True,exist_ok=True)
    current = ROOT/"python"/"brepgpu"/"__init__.py"
    baseline = args.baseline
    rows = []
    cases = []
    for backend,device in (("cpu",0),("cuda",0),("cuda",1)):
        cases.extend((("new-adapter-old-core",backend,device,current,baseline/"libbrepgpu.so",baseline/"libbrepgpu_cuda.so","unsupported"),
                      ("old-adapter-new-libraries",backend,device,baseline/"__init__.py",args.core,args.plugin,"absent")))
        if backend=="cuda":
            cases.append(("new-core-old-plugin",backend,device,current,args.core,baseline/"libbrepgpu_cuda.so","unsupported"))
    for scenario,backend,device,adapter,core,plugin,expect in cases:
        identities = tuple(digest(path) for path in (adapter,core,plugin))
        command = [sys.executable,str(Path(__file__).resolve()),"--worker","--adapter",str(adapter.resolve()),
            "--core",str(core.resolve()),"--plugin",str(plugin.resolve()),"--backend",backend,"--device",str(device),"--expect",expect]
        result = subprocess.run(command,capture_output=True,text=True,check=False)
        key = f"{scenario}-{backend}-{device}"
        (private/(key+".stdout")).write_text(result.stdout)
        (private/(key+".stderr")).write_text(result.stderr)
        if result.returncode:
            raise RuntimeError(f"compatibility case {key} failed; private logs retained")
        calls,checks,status = result.stdout.strip().split("\t")
        if identities != tuple(digest(path) for path in (adapter,core,plugin)):
            raise RuntimeError("compatibility inputs changed during case: "+key)
        rows.append((scenario,backend,device,*identities,calls,checks,status,"PASS"))
        print(key,"PASS",flush=True)
    with args.output.open("w",newline="") as stream:
        writer = csv.writer(stream,delimiter="\t",lineterminator="\n")
        writer.writerow(("scenario","backend","device","adapter_sha256","core_sha256","plugin_sha256",
                         "analytic_jet_calls","component_checks","pair_behavior","status"))
        writer.writerows(rows)


if __name__=="__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker",action="store_true")
    parser.add_argument("--adapter",type=Path)
    parser.add_argument("--core",type=Path,required=True)
    parser.add_argument("--plugin",type=Path,required=True)
    parser.add_argument("--baseline",type=Path)
    parser.add_argument("--output",type=Path)
    parser.add_argument("--backend",default="cpu")
    parser.add_argument("--device",type=int,default=0)
    parser.add_argument("--expect",choices=("ok","unsupported","absent"),default="unsupported")
    arguments = parser.parse_args()
    worker(arguments) if arguments.worker else controller(arguments)
