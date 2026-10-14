#include "brepgpu_topology.h"
#ifdef BG_TOPOLOGY_CHECK_CUDA
#include <cuda_runtime.h>
#endif
#ifdef BG_TOPOLOGY_TEST_HOOKS
#include "topology_test_hooks.h"
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
constexpr uint32_t Marker=0xababababu;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int current_device(uint32_t backend){
#ifdef BG_TOPOLOGY_CHECK_CUDA
    if(backend==BG_BACKEND_CUDA){int device=-1;
        require(cudaGetDevice(&device)==cudaSuccess,"cannot read current device for restoration witness");return device;
    }
#else
    (void)backend;
#endif
    return -1;
}
void restored(uint32_t backend,int before){
    if(before>=0)require(current_device(backend)==before,"native call failed to restore previous CUDA device");
}
struct Fixture {
    bg5_desc desc{5,sizeof(bg5_desc),BG_BACKEND_CPU,0,1,0,6,3,10};
    std::vector<uint32_t> edges{0,0,1,1,2,3,3,3,4,4};
    std::vector<uint32_t> faces{0,1,2,2,0,0,1,2,0,1};
    std::vector<int32_t> signs{-1,1,-1,1,1,-1,1,1,1,1};
    std::vector<uint32_t> offsets,members,kinds,mates,neighbors;
    bg5_timings times{};bg_diagnostic diagnostic{};
    Fixture(uint32_t backend,int device,uint32_t threads){desc.backend=backend;desc.device=device;desc.threads=threads;reset();}
    void reset(){offsets.assign(desc.edge_count+1,Marker);members.assign(desc.coedge_count,Marker);
        kinds.assign(desc.edge_count,Marker);mates.assign(desc.coedge_count,Marker);neighbors.assign(desc.coedge_count,Marker);
        std::memset(&times,0x5a,sizeof(times));std::memset(&diagnostic,0x6b,sizeof(diagnostic));}
    bg_status call(){return bg5_adjacency(&desc,sizeof(desc),edges.data(),faces.data(),signs.data(),edges.size(),
        offsets.data(),offsets.size(),members.data(),members.size(),kinds.data(),kinds.size(),
        mates.data(),neighbors.data(),mates.size(),&times,sizeof(times),&diagnostic,sizeof(diagnostic));}
    void unchanged(){for(const auto* array:{&offsets,&members,&kinds,&mates,&neighbors})
        require(std::all_of(array->begin(),array->end(),[](uint32_t x){return x==Marker;}),"preflight wrote payload output");
        const auto* bytes=reinterpret_cast<const unsigned char*>(&times);
        require(std::all_of(bytes,bytes+sizeof(times),[](unsigned char x){return x==0x5a;}),"failure published timings");}
    void qualify(){
        require(offsets==std::vector<uint32_t>({0,2,4,5,8,10,10}),"wrong literal offsets");
        require(kinds==std::vector<uint32_t>({2,3,1,5,4,0}),"wrong literal edge kinds");
        for(uint32_t e=0;e<6;++e){std::vector<uint32_t> actual(members.begin()+offsets[e],members.begin()+offsets[e+1]),expected;
            for(uint32_t c=0;c<edges.size();++c)if(edges[c]==e)expected.push_back(c);
            std::sort(actual.begin(),actual.end());require(actual==expected,"wrong literal membership");}
        const auto invalid=BG_TOPOLOGY_INVALID_SLOT;
        require(mates==std::vector<uint32_t>({1,0,3,2,invalid,invalid,invalid,invalid,invalid,invalid}),"wrong literal mates");
        require(neighbors==std::vector<uint32_t>({1,0,2,2,invalid,invalid,invalid,invalid,invalid,invalid}),"wrong literal neighbors");
        require(times.abi_version==5&&times.struct_size==sizeof(times),"wrong timing ABI");
        const double values[]={times.admission_ms,times.context_ms,times.allocate_ms,times.upload_ms,times.count_ms,
            times.scan_ms,times.fill_ms,times.reader_ms,times.download_ms,times.cleanup_ms,times.total_ms};
        for(double x:values)require(std::isfinite(x)&&x>=0,"invalid timing value");
        require(diagnostic.code==BG_OK,"successful diagnostic wrong");
    }
};

void admission(uint32_t backend,int device,uint32_t threads){
    auto rejected=[&](auto mutation){Fixture f(backend,device,threads);mutation(f);
        require(f.call()==BG_INVALID_ARGUMENT,"invalid descriptor/input accepted");f.unchanged();};
    rejected([](Fixture& f){f.desc.abi_version=4;});
    rejected([](Fixture& f){f.desc.struct_size-=4;});
    rejected([](Fixture& f){f.desc.reserved=1;});
    rejected([](Fixture& f){f.desc.backend=2;});
    rejected([](Fixture& f){f.desc.backend=BG_BACKEND_CPU;f.desc.device=1;});
    rejected([](Fixture& f){f.desc.device=-1;});
    rejected([](Fixture& f){f.desc.threads=0;});
    rejected([](Fixture& f){f.desc.threads=25;});
    rejected([](Fixture& f){f.desc.edge_count=BG_TOPOLOGY_MAX_COUNT+1;});
    rejected([](Fixture& f){f.desc.face_count=BG_TOPOLOGY_MAX_COUNT+1;});
    rejected([](Fixture& f){f.desc.coedge_count=BG_TOPOLOGY_MAX_COUNT+1;});
    rejected([](Fixture& f){f.desc.edge_count=0;});
    rejected([](Fixture& f){f.desc.face_count=0;});
    rejected([](Fixture& f){f.edges[7]=6;});
    rejected([](Fixture& f){f.faces[7]=3;});
    rejected([](Fixture& f){f.signs[7]=0;});
    rejected([](Fixture& f){f.signs[7]=2;});
    rejected([](Fixture& f){f.offsets.pop_back();});
    rejected([](Fixture& f){f.members.pop_back();});
    rejected([](Fixture& f){f.kinds.pop_back();});
    rejected([](Fixture& f){f.mates.pop_back();});
    rejected([](Fixture& f){f.edges.pop_back();});

    Fixture f(backend,device,threads);
    auto direct=[&](const bg5_desc* d,uint64_t dbytes,const uint32_t* e,uint64_t n,
        uint32_t* o,uint64_t oc,bg5_timings* t,uint64_t tb,bg_diagnostic* diag,uint64_t db){
        return bg5_adjacency(d,dbytes,e,f.faces.data(),f.signs.data(),n,o,oc,
            f.members.data(),f.members.size(),f.kinds.data(),f.kinds.size(),f.mates.data(),f.neighbors.data(),f.mates.size(),t,tb,diag,db);
    };
    require(direct(nullptr,sizeof(f.desc),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"null descriptor accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc)-1,f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"short descriptor accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times)-1,&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"short timings accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic)-1)==BG_INVALID_ARGUMENT,"short diagnostic accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),nullptr,1)==BG_INVALID_ARGUMENT,"null nonzero diagnostic accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),nullptr,10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"null nonempty input accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,nullptr,7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"null nonempty output accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,f.members.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"overlapping outputs accepted");f.unchanged();
    const auto saved_edges=f.edges;
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,f.edges.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"input output alias accepted");
    require(f.edges==saved_edges,"aliased input modified");f.unchanged();
    alignas(8) std::array<unsigned char,512> area{};std::fill(area.begin(),area.end(),0x7c);
    auto* aliased=reinterpret_cast<bg_diagnostic*>(area.data());auto* slots=reinterpret_cast<uint32_t*>(area.data());
    require(direct(&f.desc,sizeof(f.desc),slots,10,f.offsets.data(),7,&f.times,sizeof(f.times),aliased,sizeof(*aliased))==BG_INVALID_ARGUMENT,"input diagnostic alias accepted");
    require(std::all_of(area.begin(),area.end(),[](unsigned char x){return x==0x7c;}),"unsafe diagnostic modified aliased input");f.unchanged();
    auto* unaligned=reinterpret_cast<const bg5_desc*>(area.data()+1);
    require(direct(unaligned,sizeof(f.desc),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"unaligned descriptor accepted");f.unchanged();
    auto* unaligned_output=reinterpret_cast<uint32_t*>(area.data()+1);
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),10,unaligned_output,7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"unaligned output accepted");f.unchanged();
    auto* wrapped=reinterpret_cast<const uint32_t*>(std::numeric_limits<uintptr_t>::max()-3);
    require(direct(&f.desc,sizeof(f.desc),wrapped,10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"wrapped input range accepted");f.unchanged();
    require(direct(&f.desc,sizeof(f.desc),f.edges.data(),std::numeric_limits<uint64_t>::max(),f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"input multiplication overflow accepted");f.unchanged();
    require(direct(&f.desc,std::numeric_limits<uint64_t>::max(),f.edges.data(),10,f.offsets.data(),7,&f.times,sizeof(f.times),&f.diagnostic,sizeof(f.diagnostic))==BG_INVALID_ARGUMENT,"descriptor range wrap accepted");f.unchanged();
}

void permitted_inputs(uint32_t backend,int device,uint32_t threads){
    const bg5_desc desc{5,sizeof(bg5_desc),backend,device,threads,0,1,1,2};
    const uint32_t slots[2]={0,0};const int32_t signs[2]={-1,1};
    uint32_t offsets[2],members[2],kinds[1],mates[2],neighbors[2];bg5_timings times;
    require(bg5_adjacency(&desc,sizeof(desc),slots,slots,signs,2,offsets,2,members,2,kinds,1,
        mates,neighbors,2,&times,sizeof(times),nullptr,0)==BG_OK,"permitted input overlap or omitted diagnostic refused");
    require(offsets[0]==0&&offsets[1]==2&&kinds[0]==BG_TOPOLOGY_SAME_FACE_PAIR&&mates[0]==1&&mates[1]==0
        &&neighbors[0]==0&&neighbors[1]==0,"permitted input overlap result wrong");
}

void device_restoration(uint32_t backend,int requested,uint32_t threads){
#ifdef BG_TOPOLOGY_CHECK_CUDA
    if(backend!=BG_BACKEND_CUDA)return;
    const int original=current_device(backend);int count=0;
    require(cudaGetDeviceCount(&count)==cudaSuccess&&count>0,"cannot enumerate restoration witness devices");
    const int prior=count>1?(requested+1)%count:requested;
    struct RestoreOriginal {int device;~RestoreOriginal(){cudaSetDevice(device);}} guard{original};
    require(cudaSetDevice(prior)==cudaSuccess,"cannot select restoration witness prior device");
    Fixture f(backend,requested,threads);const int before=current_device(backend);
    require(before==prior,"restoration witness prior selection wrong");
    require(f.call()==BG_OK,"device restoration success witness call failed");restored(backend,before);f.qualify();
#ifdef BG_TOPOLOGY_TEST_HOOKS
    for(uint32_t point:{3u,4u,5u,6u,7u,8u}){
        f.reset();bg5_test_fail_after(point,1,0);
        const auto expected=point==3?BG_OUT_OF_MEMORY:point==8?BG_DEVICE_FAILURE:BG_INTERNAL_ERROR;
        require(f.call()==expected,"device restoration fault witness status wrong");restored(backend,before);
        require(bg5_test_live_resources()==0,"restoration fault witness leaked resource");
        bg5_test_fail_after(0,0,0);f.reset();require(f.call()==BG_OK,"device restoration fault healthy retry failed");
        restored(backend,before);f.qualify();
    }
#endif
    std::printf("device restoration witnessed prior=%d requested=%d distinct=%u\n",prior,requested,prior!=requested?1u:0u);
#else
    (void)backend;(void)requested;(void)threads;
#endif
}

void empty_and_scan(uint32_t backend,int device,uint32_t threads){
    for(uint64_t edge_count:{0ull,1ull,255ull,256ull,257ull,65537ull}){
        Fixture f(backend,device,threads);f.desc.edge_count=edge_count;f.desc.face_count=0;f.desc.coedge_count=0;
        f.edges.clear();f.faces.clear();f.signs.clear();f.reset();
        require(f.call()==BG_OK,"empty incidence rejected");
        require(std::all_of(f.offsets.begin(),f.offsets.end(),[](uint32_t x){return x==0;}),"empty scan not all zero");
        require(std::all_of(f.kinds.begin(),f.kinds.end(),[](uint32_t x){return x==BG_TOPOLOGY_UNUSED;}),"empty edge kind not unused");
    }
    Fixture large(backend,device,threads);large.desc.edge_count=65537;large.desc.face_count=1;
    large.desc.coedge_count=131074;large.edges.resize(large.desc.coedge_count);
    large.faces.assign(large.desc.coedge_count,0);large.signs.resize(large.desc.coedge_count);
    for(uint32_t c=0;c<large.desc.coedge_count;++c){large.edges[c]=c/2;large.signs[c]=(c%2)?1:-1;}
    large.reset();require(large.call()==BG_OK,"multi-level scan rejected");
    for(uint32_t e=0;e<large.desc.edge_count;++e){
        require(large.offsets[e]==2*e&&large.kinds[e]==BG_TOPOLOGY_SAME_FACE_PAIR,"multi-level scan offsets/kinds wrong");
        const auto a=large.members[2*e],b=large.members[2*e+1];
        require((a==2*e&&b==2*e+1)||(a==2*e+1&&b==2*e),"multi-level membership wrong");
        require(large.mates[2*e]==2*e+1&&large.mates[2*e+1]==2*e,"multi-level reader wrong");
    }
    require(large.offsets.back()==large.desc.coedge_count,"multi-level final offset wrong");
    Fixture hot(backend,device,threads);hot.desc.edge_count=1;hot.desc.face_count=1;hot.desc.coedge_count=65537;
    hot.edges.assign(hot.desc.coedge_count,0);hot.faces.assign(hot.desc.coedge_count,0);hot.signs.assign(hot.desc.coedge_count,1);hot.reset();
    require(hot.call()==BG_OK,"high-degree construction rejected");std::sort(hot.members.begin(),hot.members.end());
    for(uint32_t c=0;c<hot.desc.coedge_count;++c)require(hot.members[c]==c&&hot.mates[c]==UINT32_MAX&&hot.neighbors[c]==UINT32_MAX,"high-degree membership/reader wrong");
    require(hot.kinds[0]==BG_TOPOLOGY_MULTIPLE_USES&&hot.offsets.back()==hot.desc.coedge_count,"high-degree classification wrong");
}

#ifdef BG_TOPOLOGY_TEST_HOOKS
void faults(uint32_t backend,int device){
    const auto run=[&](uint32_t point,uint64_t ordinal,bg_status expected,bool secondary){
        Fixture f(backend,device,24);const auto original_times=f.times;
        bg5_test_fail_after(point,ordinal,secondary?1:0);
        const int before=current_device(backend);
        require(f.call()==expected,"fault did not report expected primary status");
        restored(backend,before);
        require(std::memcmp(&f.times,&original_times,sizeof(f.times))==0,"fault published timings");
        if(secondary)require(std::strstr(f.diagnostic.message,"secondary")!=nullptr,"secondary cleanup diagnostic lost");
        require(bg5_test_live_resources()==0,"recoverable fault leaked real CUDA resource");
        bg5_test_fail_after(0,0,0);f.reset();require(f.call()==BG_OK,"healthy retry failed");restored(backend,before);f.qualify();
    };
    if(backend==BG_BACKEND_CPU){run(1,1,BG_OUT_OF_MEMORY,false);run(1,1,BG_OUT_OF_MEMORY,true);
        run(2,2,BG_INTERNAL_ERROR,false);run(5,1,BG_INTERNAL_ERROR,false);run(6,1,BG_INTERNAL_ERROR,false);
        run(7,1,BG_INTERNAL_ERROR,false);
    } else {
        for(uint64_t ordinal:{1ull,2ull,6ull,11ull})run(3,ordinal,BG_OUT_OF_MEMORY,false);
        run(3,6,BG_OUT_OF_MEMORY,true);run(4,1,BG_INTERNAL_ERROR,false);
        run(5,1,BG_INTERNAL_ERROR,false);run(6,1,BG_INTERNAL_ERROR,false);
        run(7,1,BG_INTERNAL_ERROR,false);run(8,1,BG_DEVICE_FAILURE,false);
    }
}
#endif
}
int main(int argc,char** argv){
    try {
        const uint32_t backend=argc>1&&std::strcmp(argv[1],"cuda")==0?BG_BACKEND_CUDA:BG_BACKEND_CPU;
        const int device=argc>2?std::atoi(argv[2]):0;const uint32_t threads=argc>3?static_cast<uint32_t>(std::atoi(argv[3])):1;
        bg5_info info{};require(bg5_abi_info(&info,sizeof(info))==BG_OK,"ABI query failed");
        require(info.abi_version==5&&info.struct_size==24&&info.desc_size==48&&info.timings_size==96&&info.diagnostic_size==288,"ABI sizes wrong");
        require(bg5_abi_info(nullptr,24)==BG_INVALID_ARGUMENT,"null ABI output accepted");
        require(bg5_abi_info(&info,23)==BG_INVALID_ARGUMENT,"short ABI output accepted");
        Fixture literal(backend,device,threads);require(literal.call()==BG_OK,"literal operation failed");literal.qualify();
        admission(backend,device,threads);permitted_inputs(backend,device,threads);empty_and_scan(backend,device,threads);
        device_restoration(backend,device,threads);
#ifdef BG_TOPOLOGY_TEST_HOOKS
        faults(backend,device);
#endif
        std::printf("native topology checks passed backend=%u device=%d threads=%u fault_hooks=%u\n",backend,device,threads,
#ifdef BG_TOPOLOGY_TEST_HOOKS
            1u
#else
            0u
#endif
        );return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"native topology assertion: %s\n",error.what());return 1;}
}
