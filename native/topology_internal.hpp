#ifndef BREPGPU_TOPOLOGY_INTERNAL_HPP
#define BREPGPU_TOPOLOGY_INTERNAL_HPP
#include "brepgpu_topology.h"
#include <chrono>
#include <cstdio>
#include <cstring>
namespace brepgpu_topology {
using Clock=std::chrono::steady_clock;
inline double milliseconds(Clock::time_point begin) {
    return std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
}
struct Operation {
    bg_status status=BG_OK;
    bg_diagnostic diagnostic{};
    Operation() noexcept {
        diagnostic.abi_version=BG_ABI_VERSION;
        diagnostic.struct_size=sizeof(diagnostic);
        diagnostic.query_index=-1;
    }
    bool ok() const noexcept { return status==BG_OK; }
    void error(bg_status code,uint32_t stage,const char* message,
               int32_t backend_code=0,uint32_t flags=0) noexcept {
        diagnostic.flags|=flags;
        if(ok()) {
            status=code;diagnostic.code=code;diagnostic.stage=stage;
            diagnostic.backend_code=backend_code;
            std::snprintf(diagnostic.message,sizeof(diagnostic.message),"%s",message);
        } else {
            const auto used=std::strlen(diagnostic.message);
            if(used<sizeof(diagnostic.message)-1)
                std::snprintf(diagnostic.message+used,sizeof(diagnostic.message)-used,
                              "; secondary: %s",message);
        }
    }
};
struct Arrays {
    const uint32_t* edges;
    const uint32_t* faces;
    const int32_t* signs;
    uint32_t* offsets;
    uint32_t* members;
    uint32_t* kinds;
    uint32_t* mates;
    uint32_t* neighbors;
};
/* Hooks exist only in the separately configured tooling build. Points describe
   branch injection, not observed environmental CUDA failures. */
enum FaultPoint { HostAllocate=1, WorkerLaunch=2, DeviceAllocate=3,
    AfterUpload=4, AfterScan=5, AfterReader=6, Cleanup=7, Restore=8 };
#ifdef BG_TOPOLOGY_TEST_HOOKS
bool fault(FaultPoint point) noexcept;
void resource_add() noexcept;
void resource_remove() noexcept;
#else
inline bool fault(FaultPoint) noexcept {return false;}
inline void resource_add() noexcept {}
inline void resource_remove() noexcept {}
#endif
#ifdef BG_TOPOLOGY_HAS_CUDA
void cuda_adjacency(const bg5_desc&,const Arrays&,bg5_timings&,Operation&) noexcept;
#endif
}
#endif
