#!/usr/bin/env python3
"""Fresh-process Python publication + cleanup fault, using the owned callback model.

Run against an uninstrumented BREPGPU_ENABLE_TEST_HOOKS build, with PYTHONPATH
and BREPGPU_LIBRARY pointing at that checkout/core. No hardware fault occurs.
"""
import ctypes as C
import sys
from unittest.mock import patch
import brepgpu as bg


def main():
    if len(sys.argv) not in (2,3):raise ValueError("usage: runtime_python_faults.py MODEL_PLUGIN [projection|hierarchy|query|discovery]")
    family=sys.argv[2] if len(sys.argv)==3 else "projection"
    if family not in ("projection","hierarchy","query","discovery"):raise ValueError("unknown operation family")
    plugin=sys.argv[1]
    model=C.CDLL(plugin)
    model.bg_fault_mode.argtypes=[C.c_int];model.bg_fault_mode.restype=None
    model.bg_fault_live.argtypes=[C.c_int];model.bg_fault_live.restype=C.c_int
    engine=bg.Engine(backend="cuda",plugin_path=plugin)
    patches=engine.create_patch_set([bg.Patch(0,0,[0.,0.,0.,1.])])
    queries=engine.create_projection_query_set(patches,[0],[0.,0.,0.],[.5,.5])
    evaluator_queries=engine.create_query_set(patches,[0],[.5,.5])
    chosen=MemoryError("owned wrapper construction failure")
    captured=[]
    def reject_result(owner,handle):
        captured.append(handle)
        raise chosen
    hierarchy=engine.create_hierarchy(patches) if family in ("query","discovery") else None
    discovery_queries=engine.create_discovery_query_set(hierarchy,[0],[0.,0.,0.]) if family=="discovery" else None
    resource,mode,kind={"projection":("ProjectionResult",18,5),"hierarchy":("Hierarchy",33,6),"query":("DiscoveryQueries",34,7),"discovery":("DiscoveryResult",35,8)}[family]
    action={"projection":lambda:engine.project(queries),"hierarchy":lambda:engine.create_hierarchy(patches),
            "query":lambda:engine.create_discovery_query_set(hierarchy,[0],[0.,0.,0.]),"discovery":lambda:engine.discover(discovery_queries)}[family]
    model.bg_fault_mode(mode)
    try:
        with patch.object(bg,resource,reject_result):
            try:action()
            except MemoryError as error:
                assert error is chosen,"primary exception identity changed"
            else:raise AssertionError("constructor did not fail")
        assert len(captured)==1
        cleanup=chosen.brepgpu_cleanup_error
        assert cleanup.code==bg.BG_DEVICE_FAILURE and cleanup.flags&1,"fatal cleanup diagnostic lost"
        assert any("flags=1" in note for note in chosen.__notes__),"fatal cleanup note lost"
        assert model.bg_fault_live(kind)==0,"unwrapped private result leaked"
        diagnostic=bg.Diagnostic()
        assert engine._lib.bg1_close(captured[0],C.byref(diagnostic),C.sizeof(diagnostic))==bg.BG_INVALID_HANDLE,"unwrapped public result survived"
        model.bg_fault_mode(0)
        try:engine.evaluate(evaluator_queries,0)
        except bg.BgError as error:assert error.code==bg.BG_CONTEXT_UNUSABLE
        else:raise AssertionError("fatal cleanup did not poison evaluator")
    finally:
        model.bg_fault_mode(0)
        try:engine.close()
        except bg.BgError as error:assert error.code==bg.BG_DEVICE_FAILURE and error.flags&1
    assert all(model.bg_fault_live(kind)==0 for kind in range(9)),"callback token leaked"
    with bg.Engine() as cpu:
        with cpu.create_patch_set([bg.Patch(0,0,[0.,0.,0.,1.])]) as p:
            with cpu.create_query_set(p,[0],[.5,.5]) as q:
                with cpu.evaluate(q,0) as r:assert r.download().vector(0,0)==(0.,0.,0.)
    print(f"Python {family} publication + fatal cleanup: PASS (owned callback model, no hardware fault)")


if __name__=="__main__":main()
