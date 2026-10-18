#ifndef BG_OM10_REFERENCE_RAW_HPP
#define BG_OM10_REFERENCE_RAW_HPP
#include "../om10/scene.hpp"
#include "../rigid_placement.hpp"
#include <map>
#include <set>
namespace bgref {
using namespace bgom10;

// Independent raw input ownership. Jet is only the projection function's
// transport layout: .used/.normal are supplied external data, not evaluated
// carriers/derivatives. No source-jet API or file exists for this entry.
struct RawScene {
  std::vector<Definition> definitions;
  std::vector<Instance> instances;
  std::vector<Jet> raw;
  std::vector<uint64_t> physical;
  std::vector<Triangle> triangles;
  std::vector<Material> materials;
  std::vector<VertexRef> vertex_refs;
  std::vector<PrimitiveRef> primitive_refs;
  SceneView view() const {
    return {definitions.data(), definitions.size(), instances.data(), instances.size(),
            nullptr, raw.size(), triangles.data(), triangles.size(), nullptr, 0,
            materials.data(), materials.size(), nullptr, 0, vertex_refs.data(),
            vertex_refs.size(), primitive_refs.data(), primitive_refs.size()};
  }
  explicit RawScene(const std::string &path) {
    Bytes b(path);
    b.magic("BGREFM01");
    require(b.u32()==1 && b.u32()==0,"raw_decode: version/flags");
    uint64_t n[5];
    for(auto &count:n)count=b.u64();
    require(b.u64()==0,"raw_decode: reserved header");
    const uint64_t sizes[5]={40,136,56,24,32};
    uint64_t total=64;
    for(int i=0;i<5;++i){
      require(n[i]<=UINT32_MAX,"raw_decode: count exceeds native slot range");
      total=checked_add(total,checked_mul(n[i],sizes[i]));
    }
    require(total==b.bytes.size(),"raw_decode: section sizes do not match file");
    definitions.resize(n[0]);instances.resize(n[1]);raw.resize(n[2]);
    physical.resize(n[2]);triangles.resize(n[3]);materials.resize(n[4]);
    for(auto &d:definitions)d={b.u64(),b.u64(),b.u64(),b.u64(),b.u64()};
    for(auto &i:instances){
      i.id=b.u64();i.definition=b.u32();i.reserved=b.u32();
      b.numbers(i.matrix,12);b.numbers(i.explosion,3);
    }
    for(size_t i=0;i<raw.size();++i){
      b.numbers(raw[i].used,3);b.numbers(raw[i].normal,3);physical[i]=b.u64();
      Vec normalized;
      if(!unit(vec(raw[i].normal),normalized))
        throw Failure("raw_decode: zero/nonfinite external normal row="+std::to_string(i));
    }
    for(auto &t:triangles){
      for(auto &v:t.vertex)v=b.u32();
      t.material=b.u32();t.face=b.u64();
    }
    for(auto &m:materials){b.numbers(m.rgb,3);m.roughness=b.number();}
    b.done();validate();
  }
  void validate(){
    uint64_t vo=0,to=0;
    std::set<uint64_t> dids,iids;
    for(const auto &d:definitions){
      require(d.id!=UINT64_MAX && dids.insert(d.id).second,"raw_decode: duplicate/reserved definition ID");
      require(d.first_vertex==vo && d.first_triangle==to,"raw_decode: nonpartitioned definition ranges");
      vo=checked_add(vo,d.vertex_count);to=checked_add(to,d.triangle_count);
      require(vo<=raw.size() && to<=triangles.size(),"raw_decode: definition range");
      std::map<uint64_t,Vec> positions;
      for(uint64_t v=d.first_vertex;v<vo;++v){
        Vec p=vec(raw[v].used);
        auto result=positions.emplace(physical[v],p);
        const Vec &prior=result.first->second;
        if(!result.second && (prior.x!=p.x || prior.y!=p.y || prior.z!=p.z))
          throw Failure("raw_decode: physical key has different XYZ row="+std::to_string(v));
      }
      for(uint64_t row=d.first_triangle;row<to;++row){
        const auto &t=triangles[row];
        bool indices=true;
        for(auto v:t.vertex)indices=indices && v<d.vertex_count;
        if(!indices || t.vertex[0]==t.vertex[1] || t.vertex[1]==t.vertex[2] || t.vertex[0]==t.vertex[2] || t.material>=materials.size())
          throw Failure("raw_decode: triangle index/material row="+std::to_string(row));
        Vec a=vec(raw[d.first_vertex+t.vertex[0]].used),
            b=vec(raw[d.first_vertex+t.vertex[1]].used),
            c=vec(raw[d.first_vertex+t.vertex[2]].used),normal;
        if(!unit(cross(sub(b,a),sub(c,a)),normal))
          throw Failure("raw_decode: collapsed/nonfinite triangle row="+std::to_string(row));
      }
    }
    require(vo==raw.size() && to==triangles.size(),"raw_decode: unowned geometry");
    for(const auto &i:instances){
      require(i.id!=UINT64_MAX && iids.insert(i.id).second,"raw_decode: duplicate/reserved occurrence ID");
      require(i.definition<definitions.size() && i.reserved==0,"raw_decode: occurrence definition/reserved");
      require(brepgpu::rigid_placement_status(i.matrix)==brepgpu::PlacementStatus::rigid,"raw_decode: nonproper/nonrigid matrix");
      const auto &d=definitions[i.definition];
      require(checked_add(vertex_refs.size(),d.vertex_count)<=UINT32_MAX &&
              checked_add(primitive_refs.size(),d.triangle_count)<=UINT32_MAX,"raw_decode: placed slot overflow");
      uint32_t base=vertex_refs.size(),slot=&i-instances.data();
      for(uint64_t v=0;v<d.vertex_count;++v)vertex_refs.push_back({static_cast<uint32_t>(d.first_vertex+v),slot});
      for(uint64_t t=0;t<d.triangle_count;++t)primitive_refs.push_back({static_cast<uint32_t>(d.first_triangle+t),slot,base,0});
    }
    for(const auto &m:materials){
      for(double c:m.rgb)require(c>=0 && c<=1,"raw_decode: material color range");
      require(m.roughness>0 && m.roughness<=1,"raw_decode: material roughness range");
    }
  }
};
struct ReferenceApi {
  bool (*create)(const RawScene *,uint32_t,void **,Error *,Times *);
  bool (*render)(void *,const Request *,FrameOutput *,Error *,Times *);
  bool (*close)(void *,Error *,Times *);
};
const ReferenceApi *reference_cpu_api();
} // namespace bgref
#endif
