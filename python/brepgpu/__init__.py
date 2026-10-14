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

def _close_unwrapped(lib,handle,error):
    """Release native publication when Python ownership construction fails."""
    try:
        diagnostic=None
        try:diagnostic=Diagnostic()
        except MemoryError:pass
        code=lib.bg1_close(handle,C.byref(diagnostic) if diagnostic is not None else None,
                           C.sizeof(diagnostic) if diagnostic is not None else 0)
        if code not in (BG_OK,BG_INVALID_HANDLE):
            cleanup=BgError(code,diagnostic)
            # Preserve the primary exception and make a cleanup failure visible.
            error.brepgpu_cleanup_error=cleanup
            if hasattr(error,"add_note"):
                details=f"flags={cleanup.flags}, backend_code={cleanup.backend_code}" if diagnostic is not None else "diagnostic unavailable"
                error.add_note(f"Native resource cleanup also failed: {cleanup}; {details}")
    except BaseException as cleanup:
        try:
            if hasattr(error,"add_note"):error.add_note(f"Native resource cleanup also failed: {cleanup}")
        except BaseException:pass

def _create_resource(engine,resource_type,name,*arguments):
    handle=C.c_uint64()
    try:
        _invoke(engine._lib,name,*arguments,C.byref(handle))
        # Keep .value conversion and constructor allocation inside the guard.
        return resource_type(engine,handle.value)
    except BaseException as error:
        _close_unwrapped(engine._lib,handle,error)
        raise

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
        try:
            _invoke(self._lib,"bg1_create_context",C.byref(desc),C.sizeof(desc),C.byref(handle))
            self.backend,self.device,self.threads,self.profile=backend,device,int(threads),bool(profile)
            super().__init__(None,handle.value)
        except BaseException as error:
            _close_unwrapped(self._lib,handle,error)
            self._handle=0
            raise
    def _check(self,resource):
        if resource._engine is not self:raise BgError(BG_INVALID_HANDLE,message="resource belongs to a different engine")
        if not resource.handle:raise BgError(BG_INVALID_HANDLE,message="resource is closed")
    def create_patch_set(self,patches):
        patches=tuple(patches)
        if any(not isinstance(p,Patch) for p in patches):raise TypeError("patches must contain Patch values")
        descriptors=(PatchDesc*len(patches))();controls=array("d")
        for i,p in enumerate(patches):descriptors[i]=_descriptor(p,len(controls)//4);controls.extend(p.controls)
        payload=_pointer(controls,C.c_double)
        return _create_resource(self,PatchSet,"bg1_create_patch_set",self.handle,descriptors,len(patches),C.sizeof(descriptors),payload,len(controls))
    def create_query_set(self,patches,patch_slots,uv):
        self._check(patches)
        if not isinstance(patches,PatchSet):raise BgError(BG_INVALID_HANDLE,message="expected patch set")
        slots=array("I",(_integer(x,32,"patch slot") for x in patch_slots));parameters=array("d",_flat(uv,2))
        sp,up=_pointer(slots,C.c_uint32),_pointer(parameters,C.c_double)
        return _create_resource(self,QuerySet,"bg1_create_query_set",patches.handle,sp,len(slots),up,len(parameters))
    def evaluate(self,queries,order=2):
        self._check(queries)
        if not isinstance(queries,QuerySet):raise BgError(BG_INVALID_HANDLE,message="expected query set")
        return _create_resource(self,Result,"bg1_evaluate",queries.handle,_integer(order,32,"order"))

    def _pairs_ready(self):
        if getattr(self,"_pairs_configured",False):return
        try:_declare_pairs(self._lib)
        except AttributeError as error:raise BgError(BG_UNSUPPORTED,message="native core has no pair extension") from error
        info=PairAbiInfo()
        code=self._lib.bg4_pair_abi_info(C.byref(info),C.sizeof(info))
        names=("abi_version","struct_size","box_size","options_size","index_info_size",
               "page_info_size","diagnostic_size","double_size","integer_size","handle_size")
        expected=(4,40,C.sizeof(PairBox),C.sizeof(PairOptionsDesc),C.sizeof(PairIndexInfo),C.sizeof(PairPageInfo),C.sizeof(Diagnostic),8,4,8)
        if code or tuple(getattr(info,name) for name in names)!=expected:
            raise BgError(BG_UNSUPPORTED,message="native pair ABI does not match this adapter")
        self._pairs_configured=True
    def create_pair_index(self,boxes):
        if not self.handle:raise BgError(BG_INVALID_HANDLE,message="engine is closed")
        self._pairs_ready()
        records=[]
        for index,row in enumerate(boxes):
            if index>=BG_PAIR_MAX_INDEX_COUNT:raise ValueError("pair index exceeds item cap")
            values=[]
            for axis,value in enumerate(row):
                if axis>=6:raise ValueError("pair box must contain six values")
                values.append(float(value))
            if len(values)!=6:raise ValueError("pair box must contain six values")
            records.append(tuple(values))
        payload=(PairBox*len(records))()
        for i,row in enumerate(records):payload[i].lower[:]=row[:3];payload[i].upper[:]=row[3:]
        return _create_resource(self,PairIndex,"bg4_create_pair_index",self.handle,
                                payload if records else None,len(records),C.sizeof(payload))
    def discover_pairs(self,left,right,start=0,max_pairs=1048576):
        if not isinstance(left,PairIndex) or not isinstance(right,PairIndex):
            raise BgError(BG_INVALID_HANDLE,message="expected two pair indexes")
        self._check(left);self._check(right);self._pairs_ready()
        desc=PairOptionsDesc();desc.start=_integer(start,64,"pair start")
        desc.max_pairs=_integer(max_pairs,32,"max pairs")
        if not 1<=desc.max_pairs<=BG_PAIR_MAX_PAIRS:raise ValueError("max pairs outside 1..1048576")
        return _create_resource(self,PairPage,"bg4_discover_pairs",left.handle,right.handle,C.byref(desc),C.sizeof(desc))

    def _projection_ready(self):
        if getattr(self,"_projection_configured",False):return
        try:_declare_projection(self._lib)
        except AttributeError as error:raise BgError(BG_UNSUPPORTED,message="native core has no projection extension") from error
        info=ProjectionAbiInfo()
        code=self._lib.bg2_projection_abi_info(C.byref(info),C.sizeof(info))
        if code or (info.abi_version,info.struct_size,info.options_size,info.info_size,info.diagnostic_size,info.field_count,info.double_size,info.integer_size,info.handle_size,info.pointer_size)!=(2,40,32,48,288,7,8,4,8,8):
            raise BgError(BG_UNSUPPORTED,message="native projection ABI does not match this adapter")
        self._projection_configured=True
    def create_projection_query_set(self,patches,patch_slots,targets,seeds):
        self._check(patches)
        if not isinstance(patches,PatchSet):raise BgError(BG_INVALID_HANDLE,message="expected patch set")
        self._projection_ready()
        slots=array("I",(_integer(x,32,"patch slot") for x in patch_slots))
        target_data=array("d",_flat(targets,3));seed_data=array("d",_flat(seeds,2))
        sp,tp,up=_pointer(slots,C.c_uint32),_pointer(target_data,C.c_double),_pointer(seed_data,C.c_double)
        return _create_resource(self,ProjectionQueries,"bg2_create_projection_query_set",patches.handle,sp,len(slots),tp,len(target_data),up,len(seed_data))
    def project(self,queries,options=None):
        self._check(queries)
        if not isinstance(queries,ProjectionQueries):raise BgError(BG_INVALID_HANDLE,message="expected projection query set")
        self._projection_ready()
        if options is None:options=ProjectionOptions()
        if not isinstance(options,ProjectionOptions):raise TypeError("options must be ProjectionOptions")
        desc=_projection_options_descriptor(options)
        return _create_resource(self,ProjectionResult,"bg2_project",queries.handle,C.byref(desc),C.sizeof(desc))

    def _discovery_ready(self):
        if getattr(self,"_discovery_configured",False):return
        try:_declare_discovery(self._lib)
        except AttributeError as error:raise BgError(BG_UNSUPPORTED,message="native core has no discovery extension") from error
        info=DiscoveryAbiInfo()
        code=self._lib.bg3_discovery_abi_info(C.byref(info),C.sizeof(info))
        sizes=(info.abi_version,info.struct_size,info.hierarchy_options_size,info.node_size,info.hierarchy_info_size,info.options_size,info.info_size,info.diagnostic_size,info.field_count,info.integer_slab_count,info.double_size,info.integer_size,info.handle_size,info.pointer_size)
        expected=(3,56,C.sizeof(HierarchyOptionsDesc),C.sizeof(HierarchyNode),C.sizeof(HierarchyInfo),C.sizeof(DiscoveryOptionsDesc),C.sizeof(DiscoveryInfo),C.sizeof(Diagnostic),8,6,8,4,8,8)
        if code or sizes!=expected:raise BgError(BG_UNSUPPORTED,message="native discovery ABI does not match this adapter")
        self._discovery_configured=True
    def create_hierarchy(self,patches,options=None):
        self._check(patches)
        if not isinstance(patches,PatchSet):raise BgError(BG_INVALID_HANDLE,message="expected patch set")
        self._discovery_ready()
        if options is None:options=HierarchyOptions()
        if not isinstance(options,HierarchyOptions):raise TypeError("options must be HierarchyOptions")
        desc=_hierarchy_options_descriptor(options)
        return _create_resource(self,Hierarchy,"bg3_create_hierarchy",patches.handle,C.byref(desc),C.sizeof(desc))
    def create_discovery_query_set(self,hierarchy,patch_slots,targets):
        self._check(hierarchy)
        if not isinstance(hierarchy,Hierarchy):raise BgError(BG_INVALID_HANDLE,message="expected hierarchy")
        self._discovery_ready()
        slots=array("I",(_integer(x,32,"patch slot") for x in patch_slots));values=array("d",_flat(targets,3))
        return _create_resource(self,DiscoveryQueries,"bg3_create_discovery_query_set",hierarchy.handle,_pointer(slots,C.c_uint32),len(slots),_pointer(values,C.c_double),len(values))
    def discover(self,queries,options=None):
        self._check(queries)
        if not isinstance(queries,DiscoveryQueries):raise BgError(BG_INVALID_HANDLE,message="expected discovery query set")
        self._discovery_ready()
        if options is None:options=DiscoveryOptions()
        if not isinstance(options,DiscoveryOptions):raise TypeError("options must be DiscoveryOptions")
        desc=_discovery_options_descriptor(options)
        return _create_resource(self,DiscoveryResult,"bg3_discover",queries.handle,C.byref(desc),C.sizeof(desc))

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


BG_PROJECTION_ABI_VERSION=2
BG_PROJECTION_FIELD_COUNT=7
BG_PROJ_STATIONARY,BG_PROJ_BAD_PATCH,BG_PROJ_BAD_PARAMETER=0,1,2
BG_PROJ_ARITHMETIC,BG_PROJ_UNSUPPORTED=3,4
BG_PROJ_ITERATION_LIMIT,BG_PROJ_LINESEARCH_LIMIT,BG_PROJ_STAGNATION,BG_PROJ_RANK_UNRESOLVED=5,6,7,8
BG_PROJ_CANDIDATE_VALID,BG_PROJ_BOUNDARY,BG_PROJ_RANK_DEFICIENT,BG_PROJ_ILL_CONDITIONED=1,2,4,8
BG_PROJ_CURVATURE_POSITIVE,BG_PROJ_CURVATURE_NEGATIVE,BG_PROJ_CURVATURE_AMBIGUOUS=16,32,64

class _ProjectionVersioned(C.Structure):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,**kwargs)
        self.abi_version=BG_PROJECTION_ABI_VERSION
        self.struct_size=C.sizeof(type(self))

class ProjectionOptionsDesc(_ProjectionVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","max_iterations","max_backtracks")]
    _fields_ += [("gradient_tolerance",C.c_double),("flags",C.c_uint32),("reserved",C.c_uint32)]

class ProjectionInfo(_ProjectionVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32),("count",C.c_uint64),("patch_count",C.c_uint64)]
    _fields_ += [(name,C.c_uint32) for name in ("backend","flags","field_count","reserved")]
    _fields_ += [("kernel_ms",C.c_double)]

class ProjectionAbiInfo(_ProjectionVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","options_size","info_size","diagnostic_size","field_count","double_size","integer_size","handle_size","pointer_size")]

@dataclass(frozen=True)
class ProjectionOptions:
    max_iterations:int=64
    max_backtracks:int=24
    gradient_tolerance:float=2.**-24
    def __post_init__(self):
        object.__setattr__(self,"max_iterations",_integer(self.max_iterations,32,"max_iterations"))
        object.__setattr__(self,"max_backtracks",_integer(self.max_backtracks,32,"max_backtracks"))
        object.__setattr__(self,"gradient_tolerance",float(self.gradient_tolerance))

def _projection_options_descriptor(options):
    desc=ProjectionOptionsDesc()
    desc.max_iterations,desc.max_backtracks=options.max_iterations,options.max_backtracks
    desc.gradient_tolerance=options.gradient_tolerance
    return desc

def _declare_projection(lib):
    U,H,D=C.c_uint64,C.c_uint64,C.POINTER(Diagnostic)
    PD,OD=C.POINTER(PatchDesc),C.POINTER(ProjectionOptionsDesc)
    FP,IP=C.POINTER(C.c_double),C.POINTER(C.c_uint32)
    declarations={
        "bg2_projection_abi_info":[C.POINTER(ProjectionAbiInfo),U],
        "bg2_create_projection_query_set":[H,IP,U,FP,U,FP,U,C.POINTER(H),D,U],
        "bg2_project":[H,OD,U,C.POINTER(H),D,U],
        "bg2_projection_info":[H,C.POINTER(ProjectionInfo),U,D,U],
        "bg2_projection_download":[H,FP,U,IP,U,IP,U,IP,U,IP,U,D,U],
        "bg2_projection_sources":[H,PD,U,U,IP,U,FP,U,FP,U,OD,U,D,U],
    }
    for name,args in declarations.items():
        function=getattr(lib,name);function.argtypes=args;function.restype=C.c_uint32

class ProjectionQueries(_Resource):pass

class ProjectionResult(_Resource):
    def info(self):
        info=ProjectionInfo()
        _invoke(self._engine._lib,"bg2_projection_info",self.handle,C.byref(info),C.sizeof(info))
        return info
    def download(self):
        info=self.info();lib=self._engine._lib
        data=array("d",[0.])*(info.count*7)
        statuses=array("I",[0])*info.count;iterations=array("I",[0])*info.count
        evaluations=array("I",[0])*info.count;flags=array("I",[0])*info.count
        dp,sp,ip,ep,fp=(_pointer(data,C.c_double),_pointer(statuses,C.c_uint32),_pointer(iterations,C.c_uint32),_pointer(evaluations,C.c_uint32),_pointer(flags,C.c_uint32))
        _invoke(lib,"bg2_projection_download",self.handle,dp,len(data),sp,len(statuses),ip,len(iterations),ep,len(evaluations),fp,len(flags))
        descriptors=(PatchDesc*info.patch_count)();slots=array("I",[0])*info.count
        targets=array("d",[0.])*(info.count*3);seeds=array("d",[0.])*(info.count*2);options=ProjectionOptionsDesc()
        slotp,tp,up=_pointer(slots,C.c_uint32),_pointer(targets,C.c_double),_pointer(seeds,C.c_double)
        _invoke(lib,"bg2_projection_sources",self.handle,descriptors,info.patch_count,C.sizeof(descriptors),slotp,len(slots),tp,len(targets),up,len(seeds),C.byref(options),C.sizeof(options))
        owned_options=ProjectionOptions(options.max_iterations,options.max_backtracks,options.gradient_tolerance)
        return ProjectionBatch(lib,info,data,statuses,iterations,evaluations,flags,descriptors,slots,targets,seeds,owned_options)

class ProjectionBatch:
    _field_names=("s","t","x","y","z","distance_squared","residual")
    def __init__(self,lib,info,data,statuses,iterations,evaluations,flags,descriptors,slots,targets,seeds,options):
        self._lib,self.info,self.count=lib,info,int(info.count)
        self.data,self.statuses,self.iterations,self.evaluation_counts,self.flags=data,statuses,iterations,evaluations,flags
        self._descriptors,self._slots=descriptors,slots
        self.targets,self.seeds,self.options=targets,seeds,options
    @property
    def lineage(self):
        sources=[Lineage(d.body_id,d.entity_kind,d.entity_id,d.revision,d.chart_id) if d.flags&1 else None for d in self._descriptors]
        return tuple(sources[slot] if slot<len(sources) else None for slot in self._slots)
    def field(self,name,index):
        if isinstance(name,str):name=self._field_names.index(name)
        name,index=operator.index(name),operator.index(index)
        if not 0<=name<7 or not 0<=index<self.count:raise IndexError("projection field index outside batch")
        return self.data[name*self.count+index]
    def uv(self,index):return (self.field(0,index),self.field(1,index))
    def point(self,index):return tuple(self.field(axis+2,index) for axis in range(3))
    def distance_squared(self,index):return self.field(5,index)
    def residual(self,index):return self.field(6,index)
    def require_stationary(self):
        failures=[(i,int(status)) for i,status in enumerate(self.statuses) if status!=BG_PROJ_STATIONARY]
        if failures:raise QueryError(failures)
        return self

BG_DISCOVERY_ABI_VERSION=3
BG_BOUND_AABB,BG_BOUND_DOP26,BG_BOUND_BERNSTEIN=0,1,2
BG_HIERARCHY_LEAF=1
BG_DISC_GAP_SATISFIED,BG_DISC_UNRESOLVED,BG_DISC_BAD_PATCH,BG_DISC_BAD_TARGET,BG_DISC_ARITHMETIC=range(5)
BG_DISC_CANDIDATE_VALID,BG_DISC_GAP,BG_DISC_COVERAGE_COMPLETE=1,2,4
BG_DISC_VISIT_LIMIT,BG_DISC_REFINEMENT_LIMIT,BG_DISC_ARITHMETIC_REASON=8,16,32
class _DiscoveryVersioned(C.Structure):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,**kwargs)
        self.abi_version=3;self.struct_size=C.sizeof(type(self))
class HierarchyOptionsDesc(_DiscoveryVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","depth","reserved")]
    _fields_ += [("max_nodes",C.c_uint64),("flags",C.c_uint32),("reserved2",C.c_uint32)]
class HierarchyNode(C.Structure):
    _fields_=[("lower",C.c_double*3),("upper",C.c_double*3)]
    _fields_ += [(name,C.c_uint32) for name in ("left","right","patch_slot","u_index","v_index","u_level","v_level","flags")]
class HierarchyInfo(_DiscoveryVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32),("patch_count",C.c_uint64),("node_count",C.c_uint64)]
    _fields_ += [(name,C.c_uint32) for name in ("depth","backend","flags","node_size")]
class DiscoveryOptionsDesc(_DiscoveryVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","max_visits","max_refinements")]
    _fields_ += [("tolerance_squared",C.c_double),("flags",C.c_uint32),("reserved",C.c_uint32)]
class DiscoveryInfo(_DiscoveryVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32),("count",C.c_uint64),("patch_count",C.c_uint64)]
    _fields_ += [(name,C.c_uint32) for name in ("backend","flags","field_count","reserved")]
    _fields_ += [("kernel_ms",C.c_double)]
class DiscoveryAbiInfo(_DiscoveryVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","hierarchy_options_size","node_size","hierarchy_info_size","options_size","info_size","diagnostic_size","field_count","integer_slab_count","double_size","integer_size","handle_size","pointer_size")]
@dataclass(frozen=True)
class HierarchyOptions:
    depth:int=8
    max_nodes:int=1048576
    mode:int=0
    def __post_init__(self):
        object.__setattr__(self,"depth",_integer(self.depth,32,"depth"))
        object.__setattr__(self,"max_nodes",_integer(self.max_nodes,64,"max_nodes"))
        mode={"aabb":0,"dop26":1,"bernstein":2}.get(self.mode,self.mode) if isinstance(self.mode,str) else self.mode
        object.__setattr__(self,"mode",_integer(mode,32,"bound mode"))
@dataclass(frozen=True)
class DiscoveryOptions:
    max_visits:int=8191
    max_refinements:int=8
    tolerance_squared:float=2.**-24
    def __post_init__(self):
        object.__setattr__(self,"max_visits",_integer(self.max_visits,32,"max_visits"))
        object.__setattr__(self,"max_refinements",_integer(self.max_refinements,32,"max_refinements"))
        object.__setattr__(self,"tolerance_squared",float(self.tolerance_squared))
def _hierarchy_options_descriptor(options):
    desc=HierarchyOptionsDesc();desc.depth=options.depth;desc.max_nodes=options.max_nodes;desc.flags=options.mode;return desc
def _discovery_options_descriptor(options):
    desc=DiscoveryOptionsDesc();desc.max_visits=options.max_visits;desc.max_refinements=options.max_refinements;desc.tolerance_squared=options.tolerance_squared;return desc
def _declare_discovery(lib):
    H,U,D=C.c_uint64,C.c_uint64,C.POINTER(Diagnostic)
    P,I=C.POINTER(C.c_double),C.POINTER(C.c_uint32)
    HD,OD=C.POINTER(HierarchyOptionsDesc),C.POINTER(DiscoveryOptionsDesc)
    specs={
        "bg3_discovery_abi_info":[C.POINTER(DiscoveryAbiInfo),U],
        "bg3_create_hierarchy":[H,HD,U,C.POINTER(H),D,U],
        "bg3_hierarchy_info":[H,C.POINTER(HierarchyInfo),U,D,U],
        "bg3_hierarchy_download":[H,C.POINTER(HierarchyNode),U,U,I,U,D,U],
        "bg3_hierarchy_bound_download":[H,P,U,D,U],
        "bg3_create_discovery_query_set":[H,I,U,P,U,C.POINTER(H),D,U],
        "bg3_discover":[H,OD,U,C.POINTER(H),D,U],
        "bg3_discovery_info":[H,C.POINTER(DiscoveryInfo),U,D,U],
        "bg3_discovery_download":[H,P,U,I,U,I,U,I,U,I,U,I,U,I,U,D,U],
        "bg3_discovery_sources":[H,C.POINTER(PatchDesc),U,U,I,U,P,U,HD,U,OD,U,D,U],
    }
    for name,args in specs.items():function=getattr(lib,name);function.argtypes=args;function.restype=C.c_uint32
class Hierarchy(_Resource):
    def info(self):
        out=HierarchyInfo();_invoke(self._engine._lib,"bg3_hierarchy_info",self.handle,C.byref(out),C.sizeof(out));return out
    def download(self):
        info=self.info();lib=self._engine._lib
        nodes=(HierarchyNode*info.node_count)();roots=array("I",[0])*info.patch_count
        _invoke(lib,"bg3_hierarchy_download",self.handle,nodes,info.node_count,C.sizeof(nodes),_pointer(roots,C.c_uint32),len(roots))
        stride=(0,20,490)[info.flags];bounds=array("d",[0.])*(info.node_count*stride)
        _invoke(lib,"bg3_hierarchy_bound_download",self.handle,_pointer(bounds,C.c_double),len(bounds))
        return HierarchyBatch(info,nodes,roots,bounds)
class HierarchyBatch:
    def __init__(self,info,nodes,roots,bounds):
        self.info,self.nodes,self.roots,self.bounds=info,nodes,roots,bounds
        self.mode,self.node_count=int(info.flags),int(info.node_count)
    def box(self,index):return tuple(self.nodes[index].lower),tuple(self.nodes[index].upper)
    def cell(self,index):
        n=self.nodes[index];return (int(n.u_level),int(n.u_index),int(n.v_level),int(n.v_index))
class DiscoveryQueries(_Resource):pass
class DiscoveryResult(_Resource):
    def info(self):
        out=DiscoveryInfo();_invoke(self._engine._lib,"bg3_discovery_info",self.handle,C.byref(out),C.sizeof(out));return out
    def download(self):
        info=self.info();lib=self._engine._lib;n=int(info.count)
        data=array("d",[0.])*(n*8);slabs=[array("I",[0])*n for _ in range(6)]
        args=[self.handle,_pointer(data,C.c_double),len(data)]
        for slab in slabs:args.extend((_pointer(slab,C.c_uint32),len(slab)))
        _invoke(lib,"bg3_discovery_download",*args)
        descriptors=(PatchDesc*info.patch_count)();slots=array("I",[0])*n;targets=array("d",[0.])*(n*3)
        hierarchy_options=HierarchyOptionsDesc();options=DiscoveryOptionsDesc()
        _invoke(lib,"bg3_discovery_sources",self.handle,descriptors,info.patch_count,C.sizeof(descriptors),_pointer(slots,C.c_uint32),n,_pointer(targets,C.c_double),len(targets),C.byref(hierarchy_options),C.sizeof(hierarchy_options),C.byref(options),C.sizeof(options))
        return DiscoveryBatch(lib,info,data,*slabs,descriptors,slots,targets,
            HierarchyOptions(hierarchy_options.depth,hierarchy_options.max_nodes,hierarchy_options.flags),
            DiscoveryOptions(options.max_visits,options.max_refinements,options.tolerance_squared))
class DiscoveryBatch:
    _field_names=("s","t","x","y","z","approximate_distance_squared","lower_squared","upper_squared")
    def __init__(self,lib,info,data,statuses,flags,visits,pruned,refinements,evaluations,descriptors,slots,targets,hierarchy_options,options):
        self._lib,self.info,self.count=lib,info,int(info.count)
        self.data,self.statuses,self.flags,self.visits,self.pruned,self.refinements,self.evaluation_counts=data,statuses,flags,visits,pruned,refinements,evaluations
        self._descriptors,self._slots=descriptors,slots
        self.targets,self.hierarchy_options,self.options=targets,hierarchy_options,options
    @property
    def lineage(self):
        sources=[Lineage(d.body_id,d.entity_kind,d.entity_id,d.revision,d.chart_id) if d.flags&1 else None for d in self._descriptors]
        return tuple(sources[slot] if slot<len(sources) else None for slot in self._slots)
    def field(self,name,index):
        if isinstance(name,str):name=self._field_names.index(name)
        name,index=operator.index(name),operator.index(index)
        if not 0<=name<8 or not 0<=index<self.count:raise IndexError("discovery field index outside batch")
        return self.data[name*self.count+index]
    def uv(self,index):return self.field(0,index),self.field(1,index)
    def point(self,index):return tuple(self.field(axis+2,index) for axis in range(3))
    def approximate_distance_squared(self,index):return self.field(5,index)
    def bracket(self,index):return self.field(6,index),self.field(7,index)
    def require_gap(self):
        failures=[(i,int(status)) for i,status in enumerate(self.statuses) if status!=BG_DISC_GAP_SATISFIED or not(self.flags[i]&BG_DISC_GAP)]
        if failures:raise QueryError(failures)
        return self


BG_PAIR_ABI_VERSION=4
BG_PAIR_MAX_INDEX_COUNT=65536
BG_PAIR_MAX_PAIRS=1048576
BG_PAIR_REACHED_END,BG_PAIR_FULL_DOMAIN,BG_PAIR_PROFILED=1,2,4
class _PairVersioned(C.Structure):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,**kwargs)
        self.abi_version=BG_PAIR_ABI_VERSION;self.struct_size=C.sizeof(type(self))
class PairBox(C.Structure):
    _fields_=[("lower",C.c_double*3),("upper",C.c_double*3)]
class PairOptionsDesc(_PairVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32),("start",C.c_uint64),
              ("max_pairs",C.c_uint32),("flags",C.c_uint32)]
class PairIndexInfo(_PairVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32),("count",C.c_uint64),
              ("backend",C.c_uint32),("reserved",C.c_uint32)]
class PairPageInfo(_PairVersioned):
    _fields_=[("abi_version",C.c_uint32),("struct_size",C.c_uint32)]
    _fields_ += [(name,C.c_uint64) for name in ("total_pairs","start","count","next_start","left_count","right_count","word_count")]
    _fields_ += [("backend",C.c_uint32),("flags",C.c_uint32),("kernel_ms",C.c_double)]
class PairAbiInfo(_PairVersioned):
    _fields_=[(name,C.c_uint32) for name in ("abi_version","struct_size","box_size","options_size",
             "index_info_size","page_info_size","diagnostic_size","double_size","integer_size","handle_size")]
def _declare_pairs(lib):
    H,U,D=C.c_uint64,C.c_uint64,C.POINTER(Diagnostic)
    B,I=C.POINTER(PairBox),C.POINTER(C.c_uint32)
    specs={
        "bg4_pair_abi_info":[C.POINTER(PairAbiInfo),U],
        "bg4_create_pair_index":[H,B,U,U,C.POINTER(H),D,U],
        "bg4_pair_index_info":[H,C.POINTER(PairIndexInfo),U,D,U],
        "bg4_pair_index_download":[H,B,U,U,D,U],
        "bg4_discover_pairs":[H,H,C.POINTER(PairOptionsDesc),U,C.POINTER(H),D,U],
        "bg4_pair_page_info":[H,C.POINTER(PairPageInfo),U,D,U],
        "bg4_pair_page_download":[H,I,U,U,D,U],
    }
    for name,args in specs.items():function=getattr(lib,name);function.argtypes=args;function.restype=C.c_uint32
class PairIndex(_Resource):
    def info(self):
        out=PairIndexInfo();_invoke(self._engine._lib,"bg4_pair_index_info",self.handle,C.byref(out),C.sizeof(out));return out
    def download(self):
        info=self.info();data=(PairBox*info.count)()
        _invoke(self._engine._lib,"bg4_pair_index_download",self.handle,data if info.count else None,info.count,C.sizeof(data))
        return tuple(tuple(box.lower)+tuple(box.upper) for box in data)
class PairPage(_Resource):
    def info(self):
        out=PairPageInfo();_invoke(self._engine._lib,"bg4_pair_page_info",self.handle,C.byref(out),C.sizeof(out));return out
    def download(self):
        info=self.info();words=array("I",[0])*info.word_count
        _invoke(self._engine._lib,"bg4_pair_page_download",self.handle,_pointer(words,C.c_uint32),len(words),len(words)*4)
        return PairPageBatch(info,words)
class PairPageBatch:
    def __init__(self,info,words):self.info,self.words=info,words
