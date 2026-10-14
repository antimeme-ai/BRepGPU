#include "topology_internal.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <new>
#include <system_error>
#include <thread>
#include <vector>
#ifdef BG_TOPOLOGY_TEST_HOOKS
#include "topology_test_hooks.h"
#endif

namespace brepgpu_topology {
static_assert(sizeof(bg5_desc)==48 && sizeof(bg5_info)==24 && sizeof(bg5_timings)==96);
static_assert(sizeof(bg_diagnostic)==288 && sizeof(uint32_t)==4 && sizeof(int32_t)==4);
#ifdef BG_TOPOLOGY_TEST_HOOKS
namespace {
std::atomic<uint32_t> failure_point{0}, secondary_failure{0};
std::atomic<uint64_t> failure_ordinal{0}, live_resources{0};
}
bool fault(FaultPoint point) noexcept {
    if(point==Cleanup && secondary_failure.exchange(0))return true;
    if(failure_point.load()!=static_cast<uint32_t>(point))return false;
    const auto ordinal=failure_ordinal.load();
    if(!ordinal)return false;
    if(failure_ordinal.fetch_sub(1)==1){failure_point.store(0);return true;}
    return false;
}
void resource_add() noexcept {live_resources.fetch_add(1);}
void resource_remove() noexcept {live_resources.fetch_sub(1);}
#endif
namespace {
struct Range {uintptr_t begin=0,end=0;};
bool range(const void* pointer,uint64_t count,uint64_t width,size_t alignment,Range& out) noexcept {
    if(count>std::numeric_limits<uint64_t>::max()/width)return false;
    const auto bytes=count*width;
    if(!bytes){out={};return true;}
    const auto begin=reinterpret_cast<uintptr_t>(pointer);
    if(!pointer||begin%alignment||bytes>std::numeric_limits<uintptr_t>::max()-begin)return false;
    out={begin,begin+static_cast<uintptr_t>(bytes)};return true;
}
bool overlaps(const Range& a,const Range& b) noexcept {
    return a.begin<a.end&&b.begin<b.end&&a.begin<b.end&&b.begin<a.end;
}
struct WorkerGroup {
    std::vector<std::thread> workers;
    ~WorkerGroup(){join();}
    void join() noexcept {for(auto& worker:workers)if(worker.joinable())worker.join();}
};
template<class Function> void parallel(uint64_t count,uint32_t requested,Function function) {
    if(!count)return;
    const auto threads=static_cast<uint32_t>(std::min<uint64_t>(requested,count));
    if(threads==1){function(0,count);return;}
    WorkerGroup group;group.workers.reserve(threads);
    for(uint32_t worker=0;worker<threads;++worker){
        if(fault(WorkerLaunch))throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
        const auto begin=count*worker/threads,end=count*(worker+1)/threads;
        group.workers.emplace_back([=,&function]() noexcept {function(begin,end);});
    }
    group.join();
}

void cpu_adjacency(const bg5_desc& desc,const Arrays& a,bg5_timings& t,Operation& op) noexcept {
    std::unique_ptr<std::atomic<uint32_t>[]> counts;
    std::unique_ptr<uint32_t[]> serial_counts;
    const bool serial=desc.threads==1;
    try {
        auto start=Clock::now();
        if(fault(HostAllocate))throw std::bad_alloc();
        if(serial)serial_counts=std::make_unique<uint32_t[]>(desc.edge_count);
        else {
            counts=std::make_unique<std::atomic<uint32_t>[]>(desc.edge_count);
            for(uint64_t e=0;e<desc.edge_count;++e)counts[e].store(0,std::memory_order_relaxed);
        }
        if(desc.coedge_count){
            std::fill_n(a.mates,desc.coedge_count,BG_TOPOLOGY_INVALID_SLOT);
            std::fill_n(a.neighbors,desc.coedge_count,BG_TOPOLOGY_INVALID_SLOT);
        }
        t.allocate_ms=milliseconds(start);

        start=Clock::now();
        if(serial){for(uint64_t c=0;c<desc.coedge_count;++c)++serial_counts[a.edges[c]];}
        else parallel(desc.coedge_count,desc.threads,[&](uint64_t begin,uint64_t end) noexcept {
            for(uint64_t c=begin;c<end;++c)counts[a.edges[c]].fetch_add(1,std::memory_order_relaxed);
        });
        t.count_ms=milliseconds(start);
        start=Clock::now();uint32_t total=0;
        for(uint64_t e=0;e<desc.edge_count;++e){
            a.offsets[e]=total;
            if(serial){total+=serial_counts[e];serial_counts[e]=0;}
            else {total+=counts[e].load(std::memory_order_relaxed);counts[e].store(0,std::memory_order_relaxed);}
        }
        a.offsets[desc.edge_count]=total;t.scan_ms=milliseconds(start);
        if(fault(AfterScan))op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"injected post-scan failure");
        if(op.ok()){
            start=Clock::now();
            const auto emit=[&](uint32_t edge,uint32_t reservation,uint64_t coedge) noexcept {
                a.members[a.offsets[edge]+reservation]=static_cast<uint32_t>(coedge);
            };
            if(serial){for(uint64_t c=0;c<desc.coedge_count;++c)emit(a.edges[c],serial_counts[a.edges[c]]++,c);}
            else parallel(desc.coedge_count,desc.threads,[&](uint64_t begin,uint64_t end) noexcept {
                for(uint64_t c=begin;c<end;++c){const auto edge=a.edges[c];
                    emit(edge,counts[edge].fetch_add(1,std::memory_order_relaxed),c);
                }
            });t.fill_ms=milliseconds(start);
            start=Clock::now();
            parallel(desc.edge_count,desc.threads,[&](uint64_t begin,uint64_t end) noexcept {
                for(uint64_t e=begin;e<end;++e){
                    const auto offset=a.offsets[e],degree=a.offsets[e+1]-offset;
                    uint32_t kind=degree==0?BG_TOPOLOGY_UNUSED:
                        degree==1?BG_TOPOLOGY_BOUNDARY:BG_TOPOLOGY_MULTIPLE_USES;
                    if(degree==2){
                        const auto left=a.members[offset],right=a.members[offset+1];
                        if(a.signs[left]==a.signs[right])kind=BG_TOPOLOGY_ORIENTATION_CONFLICT;
                        else {
                            kind=a.faces[left]==a.faces[right]?BG_TOPOLOGY_SAME_FACE_PAIR:BG_TOPOLOGY_PAIRED;
                            a.mates[left]=right;a.mates[right]=left;
                            a.neighbors[left]=a.faces[right];a.neighbors[right]=a.faces[left];
                        }
                    }
                    a.kinds[e]=kind;
                }
            });t.reader_ms=milliseconds(start);
            if(fault(AfterReader))op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"injected post-reader failure");
        }
    } catch(const std::bad_alloc&){op.error(BG_OUT_OF_MEMORY,BG_STAGE_EVALUATE,"CPU allocation failed");}
    catch(const std::system_error&){op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"CPU worker launch failed; launched workers joined");}
    catch(...){op.error(BG_INTERNAL_ERROR,BG_STAGE_EVALUATE,"unexpected CPU operation failure");}
    const auto cleanup=Clock::now();counts.reset();serial_counts.reset();
    if(fault(Cleanup))op.error(BG_INTERNAL_ERROR,BG_STAGE_CLOSE,"injected cleanup report after real CPU cleanup");
    t.cleanup_ms=milliseconds(cleanup);
}
}
}

#ifdef BG_TOPOLOGY_TEST_HOOKS
extern "C" void bg5_test_fail_after(uint32_t point,uint64_t occurrence,uint32_t secondary_cleanup){
    using namespace brepgpu_topology;
    failure_point.store(point);failure_ordinal.store(occurrence);secondary_failure.store(secondary_cleanup);
}
extern "C" uint64_t bg5_test_live_resources(){return brepgpu_topology::live_resources.load();}
#endif

extern "C" bg_status bg5_abi_info(bg5_info* info,uint64_t bytes){
    using namespace brepgpu_topology;
    Range r;
    if(bytes<sizeof(bg5_info)||!range(info,bytes,1,alignof(bg5_info),r))return BG_INVALID_ARGUMENT;
    *info={5,sizeof(bg5_info),sizeof(bg5_desc),sizeof(bg5_timings),sizeof(bg_diagnostic),
#ifdef BG_TOPOLOGY_HAS_CUDA
        1
#else
        0
#endif
    };return BG_OK;
}

extern "C" bg_status bg5_adjacency(const bg5_desc* desc,uint64_t desc_bytes,
    const uint32_t* edges,const uint32_t* faces,const int32_t* signs,uint64_t input_count,
    uint32_t* offsets,uint64_t offset_count,uint32_t* members,uint64_t member_count,
    uint32_t* kinds,uint64_t kind_count,uint32_t* mates,uint32_t* neighbors,uint64_t reader_count,
    bg5_timings* timings,uint64_t timing_bytes,bg_diagnostic* diagnostic,uint64_t diag_bytes){
    using namespace brepgpu_topology;
    const auto started=Clock::now();std::array<Range,11> ranges{};
    if(desc_bytes<sizeof(bg5_desc)||timing_bytes<sizeof(bg5_timings)
       ||(!diagnostic&&diag_bytes)||(diagnostic&&diag_bytes<sizeof(bg_diagnostic)))return BG_INVALID_ARGUMENT;
    if(!range(desc,desc_bytes,1,alignof(bg5_desc),ranges[0])
       ||!range(edges,input_count,4,alignof(uint32_t),ranges[1])
       ||!range(faces,input_count,4,alignof(uint32_t),ranges[2])
       ||!range(signs,input_count,4,alignof(int32_t),ranges[3])
       ||!range(offsets,offset_count,4,alignof(uint32_t),ranges[4])
       ||!range(members,member_count,4,alignof(uint32_t),ranges[5])
       ||!range(kinds,kind_count,4,alignof(uint32_t),ranges[6])
       ||!range(mates,reader_count,4,alignof(uint32_t),ranges[7])
       ||!range(neighbors,reader_count,4,alignof(uint32_t),ranges[8])
       ||!range(timings,timing_bytes,1,alignof(bg5_timings),ranges[9])
       ||!range(diagnostic,diag_bytes,1,alignof(bg_diagnostic),ranges[10]))return BG_INVALID_ARGUMENT;
    for(size_t i=0;i<ranges.size();++i)for(size_t j=i+1;j<ranges.size();++j)
        if(!(i>=1&&i<=3&&j>=1&&j<=3)&&overlaps(ranges[i],ranges[j]))return BG_INVALID_ARGUMENT;
    Operation op;const bg5_desc d=*desc;
    if(d.abi_version!=5||d.struct_size!=sizeof(d)||d.reserved||d.backend>BG_BACKEND_CUDA
       ||d.threads<1||d.threads>24||d.device<0||(d.backend==BG_BACKEND_CPU&&d.device!=0)
       ||d.edge_count>BG_TOPOLOGY_MAX_COUNT||d.face_count>BG_TOPOLOGY_MAX_COUNT
       ||d.coedge_count>BG_TOPOLOGY_MAX_COUNT||(d.coedge_count&&(!d.edge_count||!d.face_count))
       ||input_count!=d.coedge_count||offset_count!=d.edge_count+1||member_count!=d.coedge_count
       ||kind_count!=d.edge_count||reader_count!=d.coedge_count)
        op.error(BG_INVALID_ARGUMENT,BG_STAGE_ARGUMENT,"topology descriptor or exact extent mismatch");
    if(op.ok())for(uint64_t c=0;c<d.coedge_count;++c)
        if(edges[c]>=d.edge_count||faces[c]>=d.face_count||(signs[c]!=-1&&signs[c]!=1)){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_ARGUMENT,"topology edge/face slot or orientation invalid");
            op.diagnostic.query_index=static_cast<int64_t>(c);break;
        }
    bg5_timings t{};t.abi_version=5;t.struct_size=sizeof(t);t.admission_ms=milliseconds(started);
    const Arrays arrays{edges,faces,signs,offsets,members,kinds,mates,neighbors};
    if(op.ok()){
        if(d.backend==BG_BACKEND_CPU)cpu_adjacency(d,arrays,t,op);
        else {
#ifdef BG_TOPOLOGY_HAS_CUDA
            cuda_adjacency(d,arrays,t,op);
#else
            op.error(BG_UNSUPPORTED,BG_STAGE_CONTEXT,"standalone topology library built without CUDA");
#endif
        }
    }
    t.total_ms=milliseconds(started);
    if(op.ok())*timings=t;
    if(diagnostic)*diagnostic=op.diagnostic;
    return op.status;
}
