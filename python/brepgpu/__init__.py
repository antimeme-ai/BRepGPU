"""Owned synchronous batch geometry boundary, ABI 1 (CPython 3.12/Linux x86-64).

Set BREPGPU_LIBRARY or pass library_path for the CPU core. CUDA is requested
explicitly and its optional plugin is discovered beside the core by default.
"""
from array import array
import ctypes as C
from dataclasses import dataclass
import math
import operator
import os
from pathlib import Path
import sys

BG_ABI_VERSION = 1
BG_OK, BG_INVALID_ARGUMENT, BG_UNSUPPORTED, BG_INVALID_HANDLE = 0, 1, 2, 3
BG_OUT_OF_MEMORY, BG_BACKEND_UNAVAILABLE, BG_DEVICE_FAILURE = 4, 5, 6
BG_CONTEXT_UNUSABLE, BG_INTERNAL_ERROR = 7, 8
BG_QUERY_OK, BG_QUERY_BAD_PATCH, BG_QUERY_BAD_PARAMETER = 0, 1, 2
BG_QUERY_ARITHMETIC, BG_QUERY_UNSUPPORTED = 3, 4

class _Versioned(C.Structure):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.abi_version = BG_ABI_VERSION
        self.struct_size = C.sizeof(type(self))

class Diagnostic(_Versioned):
    _fields_ = [("abi_version",C.c_uint32),("struct_size",C.c_uint32),
                ("code",C.c_uint32),("stage",C.c_uint32),("query_index",C.c_int64),
                ("backend_code",C.c_int32),("flags",C.c_uint32),("message",C.c_char*256)]

class AbiInfo(_Versioned):
    _fields_ = [(name,C.c_uint32) for name in ("abi_version","struct_size", "context_desc_size",
                "patch_desc_size","result_info_size","diagnostic_size","double_size","handle_size","slot_size","pointer_size")]

class ContextDesc(_Versioned):
    _fields_ = [("abi_version",C.c_uint32),("struct_size",C.c_uint32),("backend",C.c_uint32),
                ("device",C.c_int32),("threads",C.c_uint32),("flags",C.c_uint32),
                ("plugin_path",C.c_char_p),("plugin_path_bytes",C.c_uint64)]

class PatchDesc(_Versioned):
    _fields_ = [(name,C.c_uint32) for name in ("abi_version","struct_size","degree_u","degree_v")]
    _fields_ += [(name,C.c_uint64) for name in ("control_offset","control_count","body_id","entity_id","revision","chart_id")]
    _fields_ += [("entity_kind",C.c_uint32),("flags",C.c_uint32),("uv",C.c_double*4),("origin",C.c_double*3),("scale",C.c_double)]

class ResultInfo(_Versioned):
    _fields_ = [("abi_version",C.c_uint32),("struct_size",C.c_uint32),("count",C.c_uint64),("patch_count",C.c_uint64)]
    _fields_ += [(name,C.c_uint32) for name in ("order","terms","backend","flags")]
    _fields_ += [("kernel_ms",C.c_double)]

class BgError(RuntimeError):
    def __init__(self, code, diagnostic=None, message=None):
        self.code = int(code)
        self.stage = int(diagnostic.stage) if diagnostic else 0
        self.backend_code = int(diagnostic.backend_code) if diagnostic else 0
        self.flags = int(diagnostic.flags) if diagnostic else 0
        self.query_index = int(diagnostic.query_index) if diagnostic else -1
        self.message = message or (bytes(diagnostic.message).split(b"\0",1)[0].decode("utf-8","replace") if diagnostic else "native operation failed")
        super().__init__(f"BRepGPU error {self.code}, stage {self.stage}: {self.message}")

class QueryError(RuntimeError):
    def __init__(self, failures):
        self.failures = tuple(failures)
        super().__init__(f"{len(self.failures)} geometry queries failed; first: {self.failures[:4]}")

def _integer(value, bits, label):
    value = operator.index(value)
    if not 0 <= value < (1 << bits):
        raise ValueError(f"{label} does not fit uint{bits}")
    return value

@dataclass(frozen=True)
class Lineage:
    body_id: int = 0
    entity_kind: int = 0
    entity_id: int = 0
    revision: int = 0
    chart_id: int = 0
    def __post_init__(self):
        for name in ("body_id","entity_id","revision","chart_id","entity_kind"):
            object.__setattr__(self,name,_integer(getattr(self,name),32 if name=="entity_kind" else 64,name))

def _flat(values, width):
    values = tuple(values)
    if values and isinstance(values[0], (tuple,list,array,memoryview)):
        records = []
        for row in values:
            row = tuple(row)
            if len(row) != width: raise ValueError(f"expected {width} values per record")
            records.extend(float(value) for value in row)
        return tuple(records)
    return tuple(float(value) for value in values)

@dataclass(frozen=True)
class Patch:
    degree_u: int
    degree_v: int
    controls: object
    lineage: Lineage | None = None
    uv: tuple = (0.,1.,0.,1.)
    origin: tuple = (0.,0.,0.)
    scale: float = 1.
    def __post_init__(self):
        object.__setattr__(self,"degree_u",_integer(self.degree_u,32,"degree_u"))
        object.__setattr__(self,"degree_v",_integer(self.degree_v,32,"degree_v"))
        object.__setattr__(self,"controls",_flat(self.controls,4))
        if len(self.controls)%4: raise ValueError("homogeneous controls need four values per record")
        if self.lineage is not None and not isinstance(self.lineage,Lineage): raise TypeError("lineage must be Lineage or None")
        for name,width in (("uv",4),("origin",3)):
            values=tuple(float(x) for x in getattr(self,name))
            if len(values)!=width: raise ValueError(f"{name} needs {width} values")
            object.__setattr__(self,name,values)
        object.__setattr__(self,"scale",float(self.scale))

def _declare(lib):
    U,H=C.c_uint64,C.c_uint64
    D=C.POINTER(Diagnostic)
    declarations={
        "bg1_abi_info":[C.POINTER(AbiInfo),U],
        "bg1_create_context":[C.POINTER(ContextDesc),U,C.POINTER(H),D,U],
        "bg1_create_patch_set":[H,C.POINTER(PatchDesc),U,U,C.POINTER(C.c_double),U,C.POINTER(H),D,U],
        "bg1_create_query_set":[H,C.POINTER(C.c_uint32),U,C.POINTER(C.c_double),U,C.POINTER(H),D,U],
        "bg1_evaluate":[H,C.c_uint32,C.POINTER(H),D,U],
        "bg1_result_info":[H,C.POINTER(ResultInfo),U,D,U],
        "bg1_download":[H,C.POINTER(C.c_double),U,C.POINTER(C.c_uint32),U,D,U],
        "bg1_result_sources":[H,C.POINTER(PatchDesc),U,U,C.POINTER(C.c_uint32),U,C.POINTER(C.c_double),U,D,U],
        "bg1_close":[H,D,U],
        "bg1_convert":[C.POINTER(PatchDesc),U,U,C.POINTER(C.c_uint32),U,C.c_uint32,C.POINTER(C.c_double),U,C.POINTER(C.c_uint32),U,C.POINTER(C.c_double),U,C.POINTER(C.c_uint32),U,D,U],
    }
    for name,arguments in declarations.items():
        function=getattr(lib,name)
        function.argtypes=arguments
        function.restype=C.c_uint32

def _invoke(lib,name,*args):
    diagnostic=Diagnostic()
    code=getattr(lib,name)(*args,C.byref(diagnostic),C.sizeof(diagnostic))
    if code:
        if code==BG_OUT_OF_MEMORY: raise MemoryError(BgError(code,diagnostic))
        raise BgError(code,diagnostic)

def _pointer(owner, ctype):
    if not len(owner): return None
    return (ctype*len(owner)).from_buffer(owner)

def _descriptor(patch, offset):
    d=PatchDesc()
    d.degree_u,d.degree_v=patch.degree_u,patch.degree_v
    d.control_offset,d.control_count=offset,len(patch.controls)//4
    d.flags=2 # The explicit default world frame is also a legitimate frame.
    if patch.lineage is not None:
        d.flags|=1
        for name in ("body_id","entity_id","revision","chart_id","entity_kind"):
            setattr(d,name,getattr(patch.lineage,name))
    d.uv[:]=patch.uv;d.origin[:]=patch.origin;d.scale=patch.scale
    return d

def _library_path(explicit):
    if explicit is not None: return os.fspath(explicit)
    if os.environ.get("BREPGPU_LIBRARY"): return os.environ["BREPGPU_LIBRARY"]
    root=Path(__file__).resolve().parents[2]
    for path in (root/"build/release/libbrepgpu.so",root/"build/libbrepgpu.so"):
        if path.is_file(): return str(path)
    raise BgError(BG_BACKEND_UNAVAILABLE,message="native CPU core absent; set BREPGPU_LIBRARY or build the library")

class _Resource:
    def __init__(self, engine, handle): self._engine,self._handle=engine,int(handle)
    @property
    def handle(self): return self._handle
    def close(self):
        handle=self._handle
        if not handle:return
        diagnostic=Diagnostic()
        lib=self._engine._lib if self._engine is not None else self._lib
        code=lib.bg1_close(handle,C.byref(diagnostic),C.sizeof(diagnostic))
        if code==BG_OUT_OF_MEMORY: raise MemoryError(BgError(code,diagnostic))
        self._handle=0
        if code not in (BG_OK,BG_INVALID_HANDLE):raise BgError(code,diagnostic)
    def __enter__(self): return self
    def __exit__(self,kind,error,traceback):
        try:self.close()
        except Exception as cleanup:
            if error is None:raise
            if hasattr(error,"add_note"):error.add_note(f"Resource cleanup also failed: {cleanup}")
        return False
    def __del__(self):
        if sys.is_finalizing():return
        try:self.close()
        except Exception:pass

class Engine(_Resource):
    def __init__(self,backend="cpu",device=0,threads=1,profile=False,library_path=None,plugin_path=None):
        self._handle=0
        if backend not in ("cpu","cuda"):raise ValueError("backend must be cpu or cuda")
        try:self._lib=C.CDLL(_library_path(library_path))
        except OSError as error:raise BgError(BG_BACKEND_UNAVAILABLE,message=str(error)) from error
        try:_declare(self._lib)
        except AttributeError as error:
            raise BgError(BG_BACKEND_UNAVAILABLE,message="native core lacks the required ABI-1 entry points") from error
        info=AbiInfo()
        code=self._lib.bg1_abi_info(C.byref(info),C.sizeof(info))
        if code or (info.context_desc_size,info.patch_desc_size,info.result_info_size,info.diagnostic_size,info.double_size,info.handle_size,info.slot_size,info.pointer_size)!=(C.sizeof(ContextDesc),C.sizeof(PatchDesc),C.sizeof(ResultInfo),C.sizeof(Diagnostic),8,8,4,8):
            raise BgError(BG_BACKEND_UNAVAILABLE,message="native ABI does not match this adapter")
        if array("d").itemsize!=8 or array("I").itemsize!=4:raise BgError(BG_UNSUPPORTED,message="unsupported native array widths")
        desc=ContextDesc()
        desc.backend=0 if backend=="cpu" else 1
        device=operator.index(device)
        if not 0<=device<(1<<31):raise ValueError("device does not fit nonnegative int32")
        desc.device=device;desc.threads=_integer(threads,32,"threads");desc.flags=int(bool(profile))
        encoded=os.fsencode(plugin_path) if plugin_path is not None else b""
        if b"\0" in encoded:raise ValueError("embedded zero in plugin path")
        desc.plugin_path=encoded or None;desc.plugin_path_bytes=len(encoded)
        handle=C.c_uint64()
        _invoke(self._lib,"bg1_create_context",C.byref(desc),C.sizeof(desc),C.byref(handle))
        self.backend,self.device,self.threads,self.profile=backend,device,int(threads),bool(profile)
        super().__init__(None,handle.value)
    def _check(self,resource):
        if resource._engine is not self:raise BgError(BG_INVALID_HANDLE,message="resource belongs to a different engine")
        if not resource.handle:raise BgError(BG_INVALID_HANDLE,message="resource is closed")
    def create_patch_set(self,patches):
        patches=tuple(patches)
        if any(not isinstance(p,Patch) for p in patches):raise TypeError("patches must contain Patch values")
        descriptors=(PatchDesc*len(patches))();controls=array("d")
        for i,p in enumerate(patches):descriptors[i]=_descriptor(p,len(controls)//4);controls.extend(p.controls)
        payload=_pointer(controls,C.c_double);handle=C.c_uint64()
        _invoke(self._lib,"bg1_create_patch_set",self.handle,descriptors,len(patches),C.sizeof(descriptors),payload,len(controls),C.byref(handle))
        return PatchSet(self,handle.value)
    def create_query_set(self,patches,patch_slots,uv):
        self._check(patches)
        if not isinstance(patches,PatchSet):raise BgError(BG_INVALID_HANDLE,message="expected patch set")
        slots=array("I",(_integer(x,32,"patch slot") for x in patch_slots));parameters=array("d",_flat(uv,2))
        sp,up=_pointer(slots,C.c_uint32),_pointer(parameters,C.c_double);handle=C.c_uint64()
        _invoke(self._lib,"bg1_create_query_set",patches.handle,sp,len(slots),up,len(parameters),C.byref(handle))
        return QuerySet(self,handle.value)
    def evaluate(self,queries,order=2):
        self._check(queries)
        if not isinstance(queries,QuerySet):raise BgError(BG_INVALID_HANDLE,message="expected query set")
        handle=C.c_uint64()
        _invoke(self._lib,"bg1_evaluate",queries.handle,_integer(order,32,"order"),C.byref(handle))
        return Result(self,handle.value)

class PatchSet(_Resource):pass
class QuerySet(_Resource):pass

class Result(_Resource):
    def info(self):
        info=ResultInfo()
        _invoke(self._engine._lib,"bg1_result_info",self.handle,C.byref(info),C.sizeof(info))
        return info
    def download(self):
        info=self.info();lib=self._engine._lib
        data=array("d",[0.])*(info.count*3*info.terms);statuses=array("I",[0])*info.count
        dp,sp=_pointer(data,C.c_double),_pointer(statuses,C.c_uint32)
        _invoke(lib,"bg1_download",self.handle,dp,len(data),sp,len(statuses))
        descriptors=(PatchDesc*info.patch_count)();slots=array("I",[0])*info.count;uv=array("d",[0.])*(2*info.count)
        slotp,uvp=_pointer(slots,C.c_uint32),_pointer(uv,C.c_double)
        _invoke(lib,"bg1_result_sources",self.handle,descriptors,info.patch_count,C.sizeof(descriptors),slotp,len(slots),uvp,len(uv))
        return JetBatch(lib,info.count,info.order,data,statuses,descriptors,slots,uv,info)

class JetBatch:
    _term_names=("S","Ss","St","Sss","Sst","Stt")
    def __init__(self,lib,count,order,data,statuses,descriptors,slots,uv,info):
        self._lib,self.count,self.order=lib,int(count),int(order)
        self.data,self.statuses=data,statuses
        self._descriptors,self._slots,self._uv=descriptors,slots,uv
        self.info=info
        self._converted=False
    @property
    def lineage(self):
        sources=[]
        for d in self._descriptors:
            sources.append(Lineage(d.body_id,d.entity_kind,d.entity_id,d.revision,d.chart_id) if d.flags&1 else None)
        return tuple(sources[slot] if slot<len(sources) else None for slot in self._slots)
    def vector(self,term,index):
        if isinstance(term,str):term=self._term_names.index(term)
        term,index=operator.index(term),operator.index(index)
        count=1 if self.order==0 else 3 if self.order==1 else 6
        if not 0<=term<count or not 0<=index<self.count:raise IndexError("jet vector index outside batch")
        return tuple(self.data[(term*3+axis)*self.count+index] for axis in range(3))
    def require_ok(self):
        failures=[(i,int(status)) for i,status in enumerate(self.statuses) if status]
        if failures:raise QueryError(failures)
        return self
    def convert(self):
        if self._converted:raise ValueError("batch is already in world/source coordinates")
        # Copy mutable downloaded host arrays before releasing the GIL.
        local=array("d",self.data);status=array("I",self.statuses)
        output=array("d",[0.])*len(local);output_status=array("I",[0])*self.count
        lp,sp,op,osp=(_pointer(local,C.c_double),_pointer(status,C.c_uint32),_pointer(output,C.c_double),_pointer(output_status,C.c_uint32))
        slots=_pointer(self._slots,C.c_uint32)
        _invoke(self._lib,"bg1_convert",self._descriptors,len(self._descriptors),C.sizeof(self._descriptors),slots,self.count,self.order,lp,len(local),sp,len(status),op,len(output),osp,len(output_status))
        batch=JetBatch(self._lib,self.count,self.order,output,output_status,self._descriptors,self._slots,self._uv,self.info)
        batch._converted=True
        return batch
