#include "topology_internal.hpp"
#include <cuda_runtime.h>
#include <array>
#include <limits>

namespace brepgpu_topology {
namespace {
constexpr unsigned Tile=256;
bool fatal(cudaError_t code) noexcept {
    switch(code){
    case cudaErrorIllegalAddress:case cudaErrorLaunchTimeout:case cudaErrorAssert:
    case cudaErrorHardwareStackError:case cudaErrorIllegalInstruction:
    case cudaErrorMisalignedAddress:case cudaErrorInvalidAddressSpace:
    case cudaErrorInvalidPc:case cudaErrorLaunchFailure:case cudaErrorECCUncorrectable:
    case cudaErrorUnknown:return true;
    default:return false;
    }
}
void check(cudaError_t code,uint32_t stage,const char* action,Operation& op) noexcept {
    if(code==cudaSuccess)return;
    char message[256];std::snprintf(message,sizeof(message),"%s: %s",action,cudaGetErrorString(code));
    const bg_status status=code==cudaErrorMemoryAllocation?BG_OUT_OF_MEMORY:
        (code==cudaErrorNoDevice||code==cudaErrorInsufficientDriver||code==cudaErrorInitializationError)?
        BG_BACKEND_UNAVAILABLE:BG_DEVICE_FAILURE;
    op.error(status,stage,message,static_cast<int32_t>(code),fatal(code)?BG_DIAG_FATAL:0);
}
struct Resources {
    Operation& op;
    cudaStream_t stream=nullptr;
    std::array<void*,32> buffers{};
    size_t count=0;
    int previous=-1;
    explicit Resources(Operation& value):op(value){}
    template<class T> void allocate(T*& target,uint64_t length) noexcept {
        if(!op.ok()||!length)return;
        if(length>std::numeric_limits<size_t>::max()/sizeof(T)){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_EVALUATE,"topology device byte extent overflow");return;
        }
        if(count>=buffers.size()){
            op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"topology allocation ledger capacity exceeded");return;
        }
        if(fault(DeviceAllocate)){
            op.error(BG_OUT_OF_MEMORY,BG_STAGE_EVALUATE,"injected device allocation refusal");return;
        }
        const auto code=cudaMalloc(reinterpret_cast<void**>(&target),static_cast<size_t>(length)*sizeof(T));
        if(code==cudaSuccess){buffers[count++]=target;resource_add();}
        else check(code,BG_STAGE_EVALUATE,"allocate topology device buffer",op);
    }
    void finish(uint32_t stage=BG_STAGE_EVALUATE) noexcept {
        if(stream)check(cudaStreamSynchronize(stream),stage,"complete topology stream",op);
    }
    void close() noexcept {
        finish(BG_STAGE_CLOSE);
        while(count){void* pointer=buffers[--count];
            const auto code=cudaFree(pointer);check(code,BG_STAGE_CLOSE,"free topology device buffer",op);
            if(code==cudaSuccess)resource_remove();
        }
        if(stream){const auto code=cudaStreamDestroy(stream);
            check(code,BG_STAGE_CLOSE,"destroy topology stream",op);
            if(code==cudaSuccess)resource_remove();stream=nullptr;
        }
        if(fault(Cleanup))op.error(BG_INTERNAL_ERROR,BG_STAGE_CLOSE,"injected cleanup report after real CUDA cleanup");
        if(previous>=0)check(cudaSetDevice(previous),BG_STAGE_CLOSE,"restore prior topology device",op);
        if(fault(Restore))op.error(BG_DEVICE_FAILURE,BG_STAGE_CLOSE,"injected restore report after actual device restoration");
    }
};
unsigned blocks(uint64_t count) noexcept {
    return static_cast<unsigned>(count/Tile+(count%Tile!=0));
}
__global__ void count_uses(const uint32_t* edges,uint64_t count,uint32_t* counts){
    const uint64_t c=uint64_t(blockIdx.x)*blockDim.x+threadIdx.x;
    if(c<count)atomicAdd(counts+edges[c],1u);
}
/* Work-efficient local tree; partial tiles load zeros and every lane reaches
   every barrier. The sum hierarchy is completed by separate stream kernels. */
__global__ void scan_tiles(const uint32_t* input,uint32_t* output,uint64_t count,uint32_t* sums){
    __shared__ uint32_t values[Tile];
    const unsigned lane=threadIdx.x;
    const uint64_t index=uint64_t(blockIdx.x)*Tile+lane;
    values[lane]=index<count?input[index]:0;
    __syncthreads();
    for(unsigned stride=1;stride<Tile;stride*=2){
        const unsigned target=(lane+1)*2*stride-1;
        if(target<Tile)values[target]+=values[target-stride];
        __syncthreads();
    }
    if(lane==0){sums[blockIdx.x]=values[Tile-1];values[Tile-1]=0;}
    __syncthreads();
    for(unsigned stride=Tile/2;stride;stride/=2){
        const unsigned target=(lane+1)*2*stride-1;
        if(target<Tile){const uint32_t left=values[target-stride];
            values[target-stride]=values[target];values[target]+=left;
        }
        __syncthreads();
    }
    if(index<count)output[index]=values[lane];
}
__global__ void add_prefix(uint32_t* output,uint64_t count,const uint32_t* prefixes){
    const uint64_t index=uint64_t(blockIdx.x)*blockDim.x+threadIdx.x;
    if(index<count)output[index]+=prefixes[index/Tile];
}
__global__ void write_final(uint32_t* offsets,uint64_t edge_count,uint32_t coedge_count){
    offsets[edge_count]=coedge_count;
}
__global__ void fill_uses(const uint32_t* edges,uint64_t count,const uint32_t* offsets,
                          uint32_t* cursors,uint32_t* members){
    const uint64_t c=uint64_t(blockIdx.x)*blockDim.x+threadIdx.x;
    if(c<count){const uint32_t edge=edges[c];
        const uint32_t target=offsets[edge]+atomicAdd(cursors+edge,1u);
        members[target]=static_cast<uint32_t>(c);
    }
}
__global__ void read_mates(const uint32_t* offsets,const uint32_t* members,
    const uint32_t* faces,const int32_t* signs,uint64_t edge_count,
    uint32_t* kinds,uint32_t* mates,uint32_t* neighbors){
    const uint64_t edge=uint64_t(blockIdx.x)*blockDim.x+threadIdx.x;
    if(edge>=edge_count)return;
    const auto offset=offsets[edge],degree=offsets[edge+1]-offset;
    uint32_t kind=degree==0?BG_TOPOLOGY_UNUSED:
        degree==1?BG_TOPOLOGY_BOUNDARY:BG_TOPOLOGY_MULTIPLE_USES;
    if(degree==2){const auto left=members[offset],right=members[offset+1];
        if(signs[left]==signs[right])kind=BG_TOPOLOGY_ORIENTATION_CONFLICT;
        else {
            kind=faces[left]==faces[right]?BG_TOPOLOGY_SAME_FACE_PAIR:BG_TOPOLOGY_PAIRED;
            mates[left]=right;mates[right]=left;
            neighbors[left]=faces[right];neighbors[right]=faces[left];
        }
    }
    kinds[edge]=kind;
}
struct ScanLevel {uint64_t count=0;uint32_t* sums=nullptr;uint32_t* prefixes=nullptr;};
void scan(const uint32_t* input,uint32_t* output,uint64_t count,
          const std::array<ScanLevel,8>& levels,size_t depth,Resources& resources) noexcept {
    if(!resources.op.ok()||!count)return;
    const auto& level=levels[depth];
    scan_tiles<<<blocks(count),Tile,0,resources.stream>>>(input,output,count,level.sums);
    check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology tile scan",resources.op);
    if(level.count>1&&resources.op.ok()){
        scan(level.sums,level.prefixes,level.count,levels,depth+1,resources);
        if(resources.op.ok()){
            add_prefix<<<blocks(count),Tile,0,resources.stream>>>(output,count,level.prefixes);
            check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology prefix addition",resources.op);
        }
    }
}
}

void cuda_adjacency(const bg5_desc& desc,const Arrays& a,bg5_timings& t,Operation& op) noexcept {
    Resources r(op);auto begin=Clock::now();
    int previous=-1;const auto get_device=cudaGetDevice(&previous);
    check(get_device,BG_STAGE_CONTEXT,"read prior topology device",op);
    if(get_device==cudaSuccess)r.previous=previous;
    int device_count=0;
    if(op.ok())check(cudaGetDeviceCount(&device_count),BG_STAGE_CONTEXT,"enumerate topology devices",op);
    if(op.ok()&&desc.device>=device_count)op.error(BG_BACKEND_UNAVAILABLE,BG_STAGE_CONTEXT,"requested topology CUDA device unavailable");
    if(op.ok())check(cudaSetDevice(desc.device),BG_STAGE_CONTEXT,"select topology CUDA device",op);
    if(op.ok()){
        const auto code=cudaStreamCreateWithFlags(&r.stream,cudaStreamNonBlocking);
        check(code,BG_STAGE_CONTEXT,"create topology stream",op);
        if(code==cudaSuccess)resource_add();
    }
    t.context_ms=milliseconds(begin);
    uint32_t *edges=nullptr,*faces=nullptr,*counts=nullptr,*cursors=nullptr,
             *offsets=nullptr,*members=nullptr,*kinds=nullptr,*mates=nullptr,*neighbors=nullptr;
    int32_t* signs=nullptr;std::array<ScanLevel,8> levels{};
    begin=Clock::now();
    r.allocate(edges,desc.coedge_count);r.allocate(faces,desc.coedge_count);r.allocate(signs,desc.coedge_count);
    r.allocate(counts,desc.edge_count);r.allocate(cursors,desc.edge_count);
    r.allocate(offsets,desc.edge_count+1);r.allocate(members,desc.coedge_count);
    r.allocate(kinds,desc.edge_count);r.allocate(mates,desc.coedge_count);r.allocate(neighbors,desc.coedge_count);
    uint64_t scan_count=desc.edge_count;
    for(size_t depth=0;scan_count&&op.ok();++depth){
        if(depth>=levels.size()){op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"topology scan hierarchy capacity exceeded");break;}
        auto& level=levels[depth];level.count=blocks(scan_count);
        r.allocate(level.sums,level.count);
        if(level.count>1)r.allocate(level.prefixes,level.count);
        scan_count=level.count>1?level.count:0;
    }
    t.allocate_ms=milliseconds(begin);
    if(op.ok()){
        begin=Clock::now();
        if(desc.coedge_count){const size_t bytes=static_cast<size_t>(desc.coedge_count)*4;
            check(cudaMemcpyAsync(edges,a.edges,bytes,cudaMemcpyHostToDevice,r.stream),BG_STAGE_UPLOAD,"upload topology edges",op);
            check(cudaMemcpyAsync(faces,a.faces,bytes,cudaMemcpyHostToDevice,r.stream),BG_STAGE_UPLOAD,"upload topology faces",op);
            check(cudaMemcpyAsync(signs,a.signs,bytes,cudaMemcpyHostToDevice,r.stream),BG_STAGE_UPLOAD,"upload topology signs",op);
        }
        r.finish(BG_STAGE_UPLOAD);t.upload_ms=milliseconds(begin);
        if(fault(AfterUpload))op.error(BG_INTERNAL_ERROR,BG_STAGE_UPLOAD,"injected post-upload failure");
    }
    if(op.ok()){
        begin=Clock::now();
        if(desc.edge_count)check(cudaMemsetAsync(counts,0,static_cast<size_t>(desc.edge_count)*4,r.stream),BG_STAGE_EVALUATE,"zero topology counts",op);
        if(desc.coedge_count&&op.ok()){
            count_uses<<<blocks(desc.coedge_count),Tile,0,r.stream>>>(edges,desc.coedge_count,counts);
            check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology count",op);
        }
        r.finish();t.count_ms=milliseconds(begin);
    }
    if(op.ok()){
        begin=Clock::now();scan(counts,offsets,desc.edge_count,levels,0,r);
        if(op.ok()){
            write_final<<<1,1,0,r.stream>>>(offsets,desc.edge_count,static_cast<uint32_t>(desc.coedge_count));
            check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology final offset",op);
        }
        r.finish();t.scan_ms=milliseconds(begin);
        if(fault(AfterScan))op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"injected post-scan failure");
    }
    if(op.ok()){
        begin=Clock::now();
        if(desc.edge_count)check(cudaMemsetAsync(cursors,0,static_cast<size_t>(desc.edge_count)*4,r.stream),BG_STAGE_EVALUATE,"zero topology cursors",op);
        if(desc.coedge_count&&op.ok()){
            fill_uses<<<blocks(desc.coedge_count),Tile,0,r.stream>>>(edges,desc.coedge_count,offsets,cursors,members);
            check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology fill",op);
        }
        r.finish();t.fill_ms=milliseconds(begin);
    }
    if(op.ok()){
        begin=Clock::now();
        if(desc.coedge_count){const size_t bytes=static_cast<size_t>(desc.coedge_count)*4;
            check(cudaMemsetAsync(mates,0xff,bytes,r.stream),BG_STAGE_EVALUATE,"initialize topology mates",op);
            check(cudaMemsetAsync(neighbors,0xff,bytes,r.stream),BG_STAGE_EVALUATE,"initialize topology neighbors",op);
        }
        if(desc.edge_count&&op.ok()){
            read_mates<<<blocks(desc.edge_count),Tile,0,r.stream>>>(offsets,members,faces,signs,desc.edge_count,kinds,mates,neighbors);
            check(cudaGetLastError(),BG_STAGE_EVALUATE,"launch topology reader",op);
        }
        r.finish();t.reader_ms=milliseconds(begin);
        if(fault(AfterReader))op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"injected post-reader failure");
    }
    if(op.ok()){
        begin=Clock::now();
        check(cudaMemcpyAsync(a.offsets,offsets,static_cast<size_t>(desc.edge_count+1)*4,cudaMemcpyDeviceToHost,r.stream),BG_STAGE_DOWNLOAD,"download topology offsets",op);
        if(desc.edge_count)check(cudaMemcpyAsync(a.kinds,kinds,static_cast<size_t>(desc.edge_count)*4,cudaMemcpyDeviceToHost,r.stream),BG_STAGE_DOWNLOAD,"download topology kinds",op);
        if(desc.coedge_count){const size_t bytes=static_cast<size_t>(desc.coedge_count)*4;
            check(cudaMemcpyAsync(a.members,members,bytes,cudaMemcpyDeviceToHost,r.stream),BG_STAGE_DOWNLOAD,"download topology members",op);
            check(cudaMemcpyAsync(a.mates,mates,bytes,cudaMemcpyDeviceToHost,r.stream),BG_STAGE_DOWNLOAD,"download topology mates",op);
            check(cudaMemcpyAsync(a.neighbors,neighbors,bytes,cudaMemcpyDeviceToHost,r.stream),BG_STAGE_DOWNLOAD,"download topology neighbors",op);
        }
        r.finish(BG_STAGE_DOWNLOAD);t.download_ms=milliseconds(begin);
    }
    begin=Clock::now();r.close();t.cleanup_ms=milliseconds(begin);
}
}
