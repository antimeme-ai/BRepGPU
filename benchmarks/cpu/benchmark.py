#!/usr/bin/env python3
"""Owned, subprocess-isolated USD BRep CPU modeling benchmark."""
import argparse
from collections import defaultdict
import csv
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import math
import os
from pathlib import Path
import platform
import resource
import statistics
import subprocess
import sys
import time
import traceback

CASES = (
    "box_100", "sphere_25", "cylinder_25", "union_boxes_10",
    "intersection_boxes_10", "difference_boxes_10", "bore_1", "bore_16",
    "bore_64", "linear_sweep_10", "rotational_sweep_3", "fillet_edge", "surface_eval_10000",
    "surface_derivatives_10000", "closest_sphere_1000",
    "tess_sphere_0.2", "tess_sphere_0.05", "tess_sphere_0.01", "usd_roundtrip_bore16",
)
SAMPLE_FIELDS = ("sample", "warmup", "status", "phase", "wall_ms", "cpu_ms", "validation_ms", "worker_max_rss_mib", "detail")

def write_table(path, fields, rows):
    with Path(path).open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)

def near(actual, expected, *, rel=1e-6, absolute=1e-8):
    if not math.isfinite(actual) or abs(actual-expected) > max(absolute, rel*abs(expected)):
        raise AssertionError(f"expected {expected:.12g}, got {actual:.12g}; rel={rel}, abs={absolute}")

def point_near(actual, expected, tolerance=1e-6):
    if len(actual) != 3 or any(not math.isfinite(x) for x in actual) or math.dist(actual, expected) > tolerance:
        raise AssertionError(f"point expected {expected}, got {actual}; tolerance={tolerance}")

def solid_check(sm, brep, volume, bounds, probes=()):
    if not brep.is_manifold_solid():
        raise AssertionError("kernel reports non-manifold solid")
    actual = brep.volume(relative_accuracy=1e-8)
    near(actual, volume)
    found = brep.bounding_box(tight=True)
    for observed, expected in zip(found, bounds, strict=True):
        point_near(observed, expected, 1e-5)
    for query, expected_distance in probes:
        closest, distance = sm.get_closest_point(brep, query)
        near(distance, expected_distance, absolute=1e-6)
        near(math.dist(query, closest), distance, absolute=1e-6)
    return f"volume={actual:.12g}; faces={brep.face_count()}; edges={brep.edge_count()}; vertices={brep.vertex_count()}"

def bored_setup(sm, count):
    side, height, radius = 64.0, 6.0, 1.5
    dim = math.isqrt(count)
    centers = [(side*(i+1)/(dim+1), side*(j+1)/(dim+1), height/2) for i in range(dim) for j in range(dim)]
    box = sm.create_box((0, 0, 0), side, side, height)
    cutters = [sm.create_cylinder((x,y,-1), radius, height+2) for x,y,z in centers]
    def operation():
        result = box
        for cutter in cutters:
            result = sm.boolean_difference(result, cutter)
        return result
    volume = side*side*height-count*math.pi*radius*radius*height
    probes = [(center,radius) for center in centers]
    return operation, volume, ((0,0,0),(side,side,height)), probes

def mesh_check(mesh, report, epsilon, directory):
    if report["failures"] or not report["has_mesh"] or not report["output_manifold"]:
        raise AssertionError(f"tessellation report failure: {report}")
    arrays = mesh.to_mesh_arrays()
    points, packed = arrays["points"], arrays["faces"]
    if not points or any(len(p)!=3 or any(not math.isfinite(x) for x in p) for p in points):
        raise AssertionError("empty or nonfinite mesh")
    faces, offset = [], 0
    while offset < len(packed):
        n = packed[offset]
        if n != 3 or offset+4 > len(packed):
            raise AssertionError(f"expected complete triangular face at packed offset {offset}")
        face = tuple(packed[offset+1:offset+4])
        if len(set(face)) != 3 or any(i<0 or i>=len(points) for i in face):
            raise AssertionError("invalid triangle indices")
        faces.append(face)
        offset += 4
    edges, links, used = defaultdict(list), defaultdict(list), set()
    volume6 = 0.0
    min_facet_radius = math.inf
    for triangle_index,face in enumerate(faces):
        a,b,c = (points[i] for i in face)
        ab,ac = tuple(y-x for x,y in zip(a,b)), tuple(y-x for x,y in zip(a,c))
        normal = (ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
        norm = math.sqrt(sum(x*x for x in normal))
        if norm <= 1e-12:
            raise AssertionError("degenerate triangle")
        distance = sum(x*y for x,y in zip(normal,a))/norm
        if distance <= 0:
            raise AssertionError("sphere triangle points inward")
        # The origin's projection can lie outside an obtuse/narrow triangle.
        # Find the closest point on the actual triangle, not its infinite plane.
        q=tuple(distance*x/norm for x in normal)
        aq=tuple(y-x for x,y in zip(a,q))
        d00=sum(x*x for x in ab);d11=sum(x*x for x in ac);d01=sum(x*y for x,y in zip(ab,ac))
        d20=sum(x*y for x,y in zip(aq,ab));d21=sum(x*y for x,y in zip(aq,ac))
        v=(d11*d20-d01*d21)/(norm*norm)
        w=(d00*d21-d01*d20)/(norm*norm)
        candidates=[]
        if v>=0 and w>=0 and v+w<=1:candidates.append(q)
        for p0,p1 in ((a,b),(b,c),(c,a)):
            edge=tuple(y-x for x,y in zip(p0,p1))
            t=max(0.0,min(1.0,-sum(x*y for x,y in zip(p0,edge))/sum(x*x for x in edge)))
            candidates.append(tuple(x+t*y for x,y in zip(p0,edge)))
        witness=min(candidates,key=lambda p:sum(x*x for x in p))
        radius=math.sqrt(sum(x*x for x in witness))
        if radius < 10-epsilon-1e-5:
            aw=tuple(y-x for x,y in zip(a,witness))
            d20=sum(x*y for x,y in zip(aw,ab));d21=sum(x*y for x,y in zip(aw,ac))
            beta=(d11*d20-d01*d21)/(norm*norm)
            gamma=(d00*d21-d01*d20)/(norm*norm)
            write_table(directory/"mesh-witness.tsv",("kind","index","x","y","z"),[
                dict(kind=kind,index=index,x=p[0],y=p[1],z=p[2])
                for kind,index,p in (("a",face[0],a),("b",face[1],b),("c",face[2],c),("witness",triangle_index,witness))])
            write_table(directory/"mesh-witness-info.tsv",("key","value"),[
                dict(key=k,value=value) for k,value in (("triangle_index",triangle_index),("requested_chord",epsilon),("radius",radius),("surface_distance",10-radius),("barycentric_a",1-beta-gamma),("barycentric_b",beta),("barycentric_c",gamma))])
            raise AssertionError(f"sphere triangle violates radial bound: radius={radius}; chord={epsilon}")
        min_facet_radius = min(min_facet_radius,radius)
        volume6 += a[0]*(b[1]*c[2]-b[2]*c[1])+a[1]*(b[2]*c[0]-b[0]*c[2])+a[2]*(b[0]*c[1]-b[1]*c[0])
        for i in range(3):
            u,v,w = face[i],face[(i+1)%3],face[(i+2)%3]
            edges[tuple(sorted((u,v)))].append((u,v))
            links[u].append((v,w))
            used.add(u)
    for p in points:
        near(math.sqrt(sum(x*x for x in p)),10,absolute=1e-5)
    for incidences in edges.values():
        if len(incidences)!=2 or incidences[0] != incidences[1][::-1]:
            raise AssertionError("unpaired or inconsistently oriented shared-index edge; no vertex welding performed")
    adjacency = defaultdict(set)
    for u,v in edges:
        adjacency[u].add(v);adjacency[v].add(u)
    def connected(graph):
        pending=[next(iter(graph))];seen=set()
        while pending:
            v=pending.pop()
            if v not in seen:
                seen.add(v);pending.extend(graph[v]-seen)
        return len(seen)==len(graph)
    if not used or len(used)!=len(points) or not connected(adjacency):
        raise AssertionError("unused vertices or disconnected mesh")
    for link in links.values():
        graph=defaultdict(set)
        for a,b in link:
            graph[a].add(b);graph[b].add(a)
        if any(len(neighbors)!=2 for neighbors in graph.values()) or not connected(graph):
            raise AssertionError("non-cyclic or disconnected vertex link")
    if len(points)-len(edges)+len(faces)!=2:
        raise AssertionError("sphere mesh Euler characteristic differs from two")
    volume=volume6/6
    exact=4*math.pi*10**3/3
    lower=4*math.pi*(10-epsilon-1e-5)**3/3
    if not lower <= volume <= exact*(1+1e-6):
        raise AssertionError(f"mesh volume {volume} outside [{lower},{exact}]")
    return f"triangles={len(faces)}; points={len(points)}; volume={volume:.12g}; relative_volume_error={(exact-volume)/exact:.9g}; min_facet_radius={min_facet_radius:.12g}"

def prepare(sm, name, directory):
    if name in ("box_100", "sphere_25", "cylinder_25"):
        if name=="box_100":
            count=100; creator=lambda:sm.create_box((0,0,0),10,20,30)
            volume=6000; bounds=((0,0,0),(10,20,30))
        elif name=="sphere_25":
            count=25;creator=lambda:sm.create_sphere((0,0,0),10)
            volume=4*math.pi*1000/3;bounds=((-10,-10,-10),(10,10,10))
        else:
            count=25;creator=lambda:sm.create_cylinder((0,0,0),3,10)
            volume=math.pi*90;bounds=((-3,-3,0),(3,3,10))
        return lambda:[creator() for _ in range(count)], lambda out:"batch="+str(count)+"; "+" | ".join(solid_check(sm,b,volume,bounds) for b in out)
    if name.endswith("boxes_10"):
        operands=[(sm.create_box((0,0,0),10,10,10),sm.create_box((5,0,0),10,10,10)) for _ in range(10)]
        if name.startswith("union"):
            function=sm.boolean_union;volume=1500;bounds=((0,0,0),(15,10,10));probes=[((12,5,5),3)]
        elif name.startswith("intersection"):
            function=sm.boolean_intersection;volume=500;bounds=((5,0,0),(10,10,10));probes=[((2,5,5),3)]
        else:
            function=sm.boolean_difference;volume=500;bounds=((0,0,0),(5,10,10));probes=[((7,5,5),2)]
        return lambda:[function(a,b) for a,b in operands],lambda out:"batch=10; "+" | ".join(solid_check(sm,b,volume,bounds,probes) for b in out)
    if name.startswith("bore_"):
        operation,volume,bounds,probes=bored_setup(sm,int(name.split("_")[1]))
        return operation,lambda out:solid_check(sm,out,volume,bounds,probes)
    if name=="linear_sweep_10":
        profiles=[sm.create_circle((0,0,0),3) for _ in range(10)]
        return lambda:[sm.linear_sweep([p],(0,0,1),10) for p in profiles],lambda out:"batch=10; "+" | ".join(solid_check(sm,b,90*math.pi,((-3,-3,0),(3,3,10))) for b in out)
    if name=="rotational_sweep_3":
        points=[(2,0,0),(4,0,0),(4,0,5),(2,0,5)]
        profiles=[[sm.create_line_segment(points[i],points[(i+1)%4]) for i in range(4)] for _ in range(3)]
        return lambda:[sm.rotational_sweep(p,(0,0,0),(0,0,1),360) for p in profiles],lambda out:"batch=3; "+" | ".join(solid_check(sm,b,60*math.pi,((-4,-4,0),(4,4,5)),[((0,0,2.5),2)]) for b in out)
    if name=="fillet_edge":
        box=sm.create_box((0,0,0),10,20,30)
        edges=[e for e in box.edges() if all(abs(p[0])+abs(p[1])<1e-8 for p in (e.start_vertex().point(),e.end_vertex().point()))]
        if len(edges)!=1:raise AssertionError(f"expected one origin vertical edge, found {len(edges)}")
        volume=6000-(1-math.pi/4)*4*30
        return lambda:sm.fillet_edges(box,edges,radius=2,xsect_type=1),lambda out:solid_check(sm,out,volume,((0,0,0),(10,20,30)),[((0,0,15),2*(math.sqrt(2)-1))])
    if name.startswith("surface_"):
        surface=sm.create_surface_from_points([(12*i/3,9*j/3,3 if i in (1,2) and j in (1,2) else 0) for i in range(4) for j in range(4)],4,4)
        params=[((i%100)/99,(i//100)/99) for i in range(10000)]
        derivatives=name.startswith("surface_derivatives")
        def oracle(out):
            for (u,v),value in zip(params,out,strict=True):
                point=(12*u,9*v,27*u*(1-u)*v*(1-v))
                if derivatives:
                    for actual,expected in zip(value,(point,(12,0,27*(1-2*u)*v*(1-v)),(0,9,27*u*(1-u)*(1-2*v))),strict=True):point_near(actual,expected,1e-7)
                else:point_near(value,point,1e-7)
            return "calls=10000; analytic bicubic Bernstein points"+(" and derivatives" if derivatives else "")
        return lambda:[surface.derivatives(u,v) if derivatives else surface.evaluate(u,v) for u,v in params],oracle
    if name=="closest_sphere_1000":
        sphere=sm.create_sphere((0,0,0),10)
        queries=[(20+i%13,1+i%7,2+i%11) for i in range(1000)]
        def oracle(out):
            for query,(point,distance) in zip(queries,out,strict=True):
                length=math.sqrt(sum(x*x for x in query))
                point_near(point,tuple(10*x/length for x in query),1e-6)
                near(distance,length-10,absolute=1e-6)
            return "calls=1000; analytic radial closest points"
        return lambda:[sm.get_closest_point(sphere,p) for p in queries],oracle
    if name.startswith("tess_sphere_"):
        sphere=sm.create_sphere((0,0,0),10)
        epsilon=float(name.rsplit("_",1)[1])
        return lambda:sphere.tessellate(chord_height_tolerance=epsilon,curve_angle_tolerance_deg=0,surface_angle_tolerance_deg=0,allow_partial=False,return_report=True),lambda out:mesh_check(out[0],out[1],epsilon,directory)
    if name=="usd_roundtrip_bore16":
        operation,volume,bounds,probes=bored_setup(sm,16)
        model=operation()
        path=directory/"bored16.usda"
        def roundtrip():
            sm.usd.export_brep(model,str(path))
            return sm.usd.import_breps(str(path))
        def oracle(out):
            if len(out)!=1:raise AssertionError(f"expected one imported BRep, got {len(out)}")
            detail=solid_check(sm,out[0],volume,bounds,probes)
            curved=sum(not face.surface().is_planar(1e-8) for face in out[0].faces())
            if curved==0:raise AssertionError("curved hole surfaces were replaced by planar surfaces")
            return detail+f"; nonplanar_faces={curved}; file_bytes={path.stat().st_size}"
        return roundtrip,oracle
    raise ValueError(name)

def worker(args):
    directory=Path(args.output);directory.mkdir(parents=True,exist_ok=True)
    current_phase="startup"
    def phase(value):
        nonlocal current_phase
        current_phase=value
        (directory/"phase.txt").write_text(value+"\n")
    os.sched_setaffinity(0,{args.cpu})
    phase("import")
    import usd_brep as sm
    samples=[]
    count=args.repeats+args.warmups
    result=operation=oracle=None
    for index in range(count):
        warmup=index<args.warmups
        row=dict(sample=index,warmup=int(warmup),status="",phase="",wall_ms="",cpu_ms="",validation_ms="",worker_max_rss_mib="",detail="")
        try:
            phase(f"setup {index}")
            result=operation=oracle=None
            operation,oracle=prepare(sm,args.worker,directory)
            phase(f"operation {index}")
            c0=time.process_time_ns();w0=time.perf_counter_ns()
            try:
                result=operation()
            finally:
                w1=time.perf_counter_ns();c1=time.process_time_ns()
                row.update(wall_ms=(w1-w0)/1e6,cpu_ms=(c1-c0)/1e6)
            phase(f"validation {index}")
            v0=time.perf_counter_ns()
            row["detail"]=oracle(result)
            row["validation_ms"]=(time.perf_counter_ns()-v0)/1e6
            row["status"]="pass"
        except Exception as error:
            if current_phase.startswith("validation"):
                row["status"]="oracle_mismatch" if isinstance(error,AssertionError) else "oracle_error"
            else:row["status"]="setup_error" if current_phase.startswith("setup") else "operation_error"
            row["detail"]=f"{type(error).__name__}: {error}"
            traceback.print_exc()
        row["phase"]=current_phase
        row["worker_max_rss_mib"]=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024
        samples.append(row)
        write_table(directory/"samples.tsv",SAMPLE_FIELDS,samples)
        print(f"{args.worker} sample {index}: {row['status']} wall_ms={row['wall_ms']}",flush=True)
        if row["status"]!="pass":
            return 1
    if isinstance(result,sm.Brep):
        phase("artifact")
        try:
            result.write_to_file(str(directory/"model.smb"))
            artifact=dict(status="written",detail="model.smb")
        except Exception as error:
            artifact=dict(status="artifact_error",detail=f"{type(error).__name__}: {error}")
        write_table(directory/"artifact.tsv",("status","detail"),[artifact])
    phase("complete")
    return 0

def orchestrate(args):
    output=Path(args.output).resolve()
    output.mkdir(parents=True,exist_ok=False)
    driver_bytes=Path(__file__).read_bytes()
    frozen_driver=output/"benchmark-source.py"
    frozen_driver.write_bytes(driver_bytes)
    selected=args.cases.split(",") if args.cases else list(CASES)
    if any(name not in CASES for name in selected):raise ValueError("unknown case")
    if args.cpu not in os.sched_getaffinity(0):raise ValueError("CPU outside allowed affinity")
    environment=dict(os.environ,CUDA_VISIBLE_DEVICES="",OMP_NUM_THREADS="1",OPENBLAS_NUM_THREADS="1",MKL_NUM_THREADS="1")
    metadata=[("started_utc",datetime.now(timezone.utc).isoformat()),("host",platform.node()),("platform",platform.platform()),("python",sys.version.replace("\n"," ")),("usd_brep",importlib.metadata.version("usd-brep")),("cpu_affinity",str(args.cpu)),("load_start",str(os.getloadavg())),("repeats",str(args.repeats)),("warmups",str(args.warmups)),("timeout_seconds",str(args.timeout)),("cuda_visible_devices","empty"),("thread_settings","OMP/OPENBLAS/MKL=1; affinity controls all native threads")]
    metadata.append(("cpu_model",next((line.split(":",1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines() if line.startswith('model name')),"unknown")))
    metadata.append(("driver_sha256",hashlib.sha256(driver_bytes).hexdigest()))
    wheels=Path(sys.prefix).parent/"wheels"
    for wheel in sorted(wheels.glob("usd_brep-1.0.0-*.whl")):
        metadata.extend([("wheel_file",str(wheel)),("wheel_sha256",hashlib.sha256(wheel.read_bytes()).hexdigest()),("wheel_source","https://pypi.org/project/usd-brep/1.0.0/")])
    # Clock overhead is a diagnostic; it is never subtracted from measurements.
    overhead=[]
    for _ in range(1000):
        c0=time.process_time_ns();w0=time.perf_counter_ns();w1=time.perf_counter_ns();c1=time.process_time_ns()
        overhead.append(w1-w0)
    metadata.append(("empty_wall_clock_median_ns",str(statistics.median(overhead))))
    topology=subprocess.run(["lscpu","-e=CPU,CORE,SOCKET,NODE,ONLINE"],capture_output=True,text=True,check=True).stdout
    (output/"cpu-topology.txt").write_text(topology)
    for entry in ("thread_siblings_list","core_id","physical_package_id"):
        metadata.append((entry,Path(f"/sys/devices/system/cpu/cpu{args.cpu}/topology/{entry}").read_text().strip()))
    write_table(output/"environment.tsv",("key","value"),[dict(key=k,value=v) for k,v in metadata])
    script=frozen_driver;rows=[]
    for name in selected:
        folder=output/name
        command=[sys.executable,str(script),"--worker",name,"--output",str(folder),"--cpu",str(args.cpu),"--repeats",str(args.repeats),"--warmups",str(args.warmups)]
        with (output/(name+".log")).open("w") as log:
            try:
                completed=subprocess.run(command,stdout=log,stderr=log,env=environment,timeout=args.timeout)
                status="pass" if completed.returncode==0 else ("fault" if completed.returncode<0 else "failed")
                exitcode=str(completed.returncode)
            except subprocess.TimeoutExpired:
                status="timeout";exitcode="timeout"
        samples=[]
        if (folder/"samples.tsv").exists():
            with (folder/"samples.tsv").open() as stream:samples=list(csv.DictReader(stream,delimiter="\t"))
        measured=[s for s in samples if s["warmup"]=="0"]
        last_phase=(folder/"phase.txt").read_text().strip() if (folder/"phase.txt").exists() else "startup"
        complete_samples=len(measured)==args.repeats and len(samples)==args.repeats+args.warmups and all(s["status"]=="pass" for s in samples)
        if last_phase=="artifact" and complete_samples:status="pass_artifact_fault"
        qualified=status in ("pass","pass_artifact_fault") and complete_samples
        if status=="pass" and not qualified:status="incomplete"
        wall=[float(s["wall_ms"]) for s in measured] if qualified else []
        cpu=[float(s["cpu_ms"]) for s in measured] if qualified else []
        detail=samples[-1]["detail"] if samples else "no completed sample"
        if (folder/"artifact.tsv").exists():
            with (folder/"artifact.tsv").open() as stream:artifact=next(csv.DictReader(stream,delimiter="\t"))
            if artifact["status"]!="written":detail+="; "+artifact["detail"];status="pass_artifact_error"
        row=dict(case=name,status=status,samples=len(measured),median_wall_ms=statistics.median(wall) if wall else "",min_wall_ms=min(wall) if wall else "",max_wall_ms=max(wall) if wall else "",median_cpu_ms=statistics.median(cpu) if cpu else "",exit_code=exitcode,last_phase=last_phase,detail=detail)
        rows.append(row)
        write_table(output/"summary.tsv",tuple(row),rows)
        print(f"{name}: {status}; median_wall_ms={row['median_wall_ms']}",flush=True)
    report=["# USD BRep CPU baseline on "+platform.node(),"",f"USD BRep {importlib.metadata.version('usd-brep')}; logical CPU {args.cpu}; {args.warmups} excluded warmup and {args.repeats} fresh-input samples per case. Timings exclude setup and validation; I/O cases include I/O. See environment.tsv and cpu-topology.txt for hardware/load/affinity.","","| Workload | Result | Median wall ms | Range ms | Median CPU ms |","| --- | --- | ---: | --- | ---: |"]
    for row in rows:
        values=[f"{row[k]:.3f}" if row[k]!="" else "—" for k in ("median_wall_ms","min_wall_ms","max_wall_ms","median_cpu_ms")]
        report.append(f"| {row['case']} | {row['status']} | {values[0]} | {values[1]}–{values[2]} | {values[3]} |")
    report.extend(["","Failed/invalid cases have no qualified timing. Detailed checks, all samples and native logs are retained per case. Primitive/Boolean batches and scalar surface/query cases include Python loop and binding overhead; elapsed time is per complete named workload. Five samples are an exploratory baseline on a shared host, not a confident ranking. Affinity constrains one logical CPU, without reserving its SMT sibling or fixing clock speed.","","## Findings",""])
    for row in rows:
        if row["status"]!="pass":report.append(f"- **{row['case']}**: {row['status']}, last phase {row['last_phase']}; {row['detail']}")
    if all(row["status"]=="pass" for row in rows):report.append("All selected cases met their declared geometric oracles.")
    report.extend(["","The tessellation oracle checks sphere-specific radial/volume bounds and mesh incidence/link/orientation/connectivity; it does not implement a general triangle self-intersection detector. Kernel manifold flags are additional diagnostics. USD round-trip topology counts are diagnostic, not equality requirements."])
    (output/"REPORT.md").write_text("\n".join(report)+"\n")
    return 0 if all(row["status"]=="pass" for row in rows) else 1

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",required=True)
    parser.add_argument("--worker",choices=CASES)
    parser.add_argument("--cases")
    parser.add_argument("--cpu",type=int,default=min(os.sched_getaffinity(0)))
    parser.add_argument("--repeats",type=int,default=5)
    parser.add_argument("--warmups",type=int,default=1)
    parser.add_argument("--timeout",type=float,default=180)
    args=parser.parse_args()
    if args.repeats<1 or args.warmups<0 or args.timeout<=0:parser.error("invalid sampling/deadline settings")
    return worker(args) if args.worker else orchestrate(args)

if __name__=="__main__":
    raise SystemExit(main())
