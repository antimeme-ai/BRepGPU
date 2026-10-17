// Owned extraction and admission; polygon diagonals belong to the official SDK.
#include "raw_mesh.hpp"
#include <SmBrep.h>
#include <SmPoly.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace {
using Edge=std::pair<uint64_t,uint64_t>;
using UserBytes=std::array<unsigned char,sizeof(ULONG)>;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Vec point(SmPolyVertex *vertex){const auto &p=vertex->GetPoint();return {p[0],p[1],p[2]};}
Vec normal(SmPolyEdge *edge){const auto &n=edge->GetAuxData()->m_vNormal;return {n[0],n[1],n[2]};}
bool finite(const Vec &v){return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);}
bool same_bytes(const Vec &a,const Vec &b){return std::memcmp(a.data(),b.data(),sizeof(Vec))==0;}
UserBytes save_user(const ULONG &value){UserBytes bytes;std::memcpy(bytes.data(),&value,sizeof(value));return bytes;}
void restore_user(ULONG &value,const UserBytes &bytes){std::memcpy(&value,bytes.data(),sizeof(value));}

bool rectangle(const std::vector<Vec> &points){
  if(points.size()!=4)return false;
  for(size_t shift=0;shift<4;++shift){
    const auto &a=points[shift],&b=points[(shift+1)%4],&c=points[(shift+2)%4],&d=points[(shift+3)%4];
    for(size_t axis=0;axis<3;++axis){
      if(a[axis]==b[axis]||a[axis]!=d[axis]||b[axis]!=c[axis])continue;
      bool matching=true,nonzero=false;
      for(size_t k=0;k<3;++k)if(k!=axis){matching=matching&&a[k]==b[k]&&c[k]==d[k];nonzero=nonzero||a[k]!=d[k];}
      if(matching&&nonzero)return true;
    }
  }
  return false;
}

SmTArray<SmPolyEdge*> ring(SmPolyFace *face){
  SmTArray<SmPolyLoop*> loops;face->GetPolyLoops(loops);
  require(loops.GetSize()==1&&loops[0]&&loops[0]->IsClosed(),"mesh_export: polygon must have one closed outer loop and no holes");
  SmTArray<SmPolyEdge*> edges;loops[0]->GetPolyEdges(edges);
  require(edges.GetSize()>0,"mesh_export: empty polygon loop");
  std::set<SmPolyEdge*> unique;
  for(ULONG i=0;i<edges.GetSize();++i){
    auto *edge=edges[i];
    require(edge&&edge->GetStartPolyVertex()&&unique.insert(edge).second&&edge->GetPolyFace()==face&&edge->GetPolyLoop()==loops[0],
      "mesh_export: polygon edge has foreign owner or repeats");
  }
  for(ULONG i=0;i<edges.GetSize();++i){
    auto *edge=edges[i];
    require(edge->GetCCWPolyEdge()==edges[(i+1)%edges.GetSize()]&&
      edge->GetCWPolyEdge()==edges[(i+edges.GetSize()-1)%edges.GetSize()]&&
      edge->GetEndPolyVertex()&&
      edge->GetEndPolyVertex()==edges[(i+1)%edges.GetSize()]->GetStartPolyVertex(),
      "mesh_export: directed polygon loop is disconnected");
  }
  return edges;
}

struct Parent {
  SmFace *source=nullptr;
  UserBytes user1{},user2{},smooth{};
  std::vector<uint64_t> boundary;
  std::map<uint64_t,Vec> normals;
  bool split=false;
};

// Every failure discards the PolyBrep. Restore valid tagged objects before its
// caller destroys it; an unknown ancestry is never guessed or exported.
class RestoreParents {
  SmPolyBrep &poly;
  const std::vector<Parent> &parents;
  bool active=true;
public:
  RestoreParents(SmPolyBrep &p,const std::vector<Parent> &v):poly(p),parents(v){}
  bool restore(){
    SmTArray<SmPolyFace*> faces;poly.GetPolyFaces(faces);bool complete=true;
    for(ULONG i=0;i<faces.GetSize();++i){
      auto *face=faces[i];
      if(!face||face->m_lUserLong1==0||face->m_lUserLong1>parents.size()){complete=false;continue;}
      const auto &parent=parents[face->m_lUserLong1-1];face->SetOriginalFace(parent.source,FALSE);
      restore_user(face->m_lUserLong1,parent.user1);restore_user(face->m_lUserLong2,parent.user2);
      restore_user(face->m_lSmoothGroup,parent.smooth);
    }
    active=false;return complete;
  }
  ~RestoreParents(){if(active)try{restore();}catch(...){/* caller discards the failed mesh */}}
};
}

Mesh extract(SmBrep &brep,SmPolyBrep &poly){
  SmTArray<SmFace*> source_faces;brep.GetFaces(source_faces);
  std::map<SmFace*,uint64_t> source_ids;
  for(ULONG i=0;i<source_faces.GetSize();++i)
    require(source_faces[i]&&source_ids.emplace(source_faces[i],i).second,"mesh_export: invalid imported face list");
  SmTArray<SmPolyVertex*> vertices;poly.GetPolyVertices(vertices);
  std::map<SmPolyVertex*,uint64_t> vertex_ids;std::vector<Vec> xyz;
  for(ULONG i=0;i<vertices.GetSize();++i){
    require(vertices[i]&&vertex_ids.emplace(vertices[i],i).second,"mesh_export: invalid physical vertex list");
    xyz.push_back(point(vertices[i]));require(finite(xyz.back()),"mesh_export: nonfinite actual vertex");
  }
  SmTArray<SmPolyFace*> original;poly.GetPolyFaces(original);
  require(original.GetSize()>0,"mesh_export: empty actual tessellation");
  std::vector<Parent> parents;parents.reserve(original.GetSize());Mesh output;
  output.physical_count=vertices.GetSize();output.original_polygons=original.GetSize();
  // Preflight all input before any mutation. Unsupported source rings leave no
  // partial exported definition and never enter the SDK triangulator.
  for(ULONG i=0;i<original.GetSize();++i){
    auto *face=original[i];require(face&&source_ids.count(face->GetOriginalFace()),"mesh_export: polygon has no original imported face identity");
    Parent parent;parent.source=face->GetOriginalFace();parent.user1=save_user(face->m_lUserLong1);
    parent.user2=save_user(face->m_lUserLong2);parent.smooth=save_user(face->m_lSmoothGroup);
    auto edges=ring(face);require(edges.GetSize()==3||edges.GetSize()==4,
      "mesh_export: unsupported nontriangle polygon; only exact axis-extruded rectangles are admitted");
    std::vector<Vec> points;
    for(ULONG j=0;j<edges.GetSize();++j){
      auto *edge=edges[j];auto key=vertex_ids.find(edge->GetStartPolyVertex());
      require(key!=vertex_ids.end(),"mesh_export: foreign physical vertex pointer");
      require(edge->GetAuxData(),"mesh_export: missing actual polygon corner normal");
      const auto n=normal(edge);require(finite(n)&&std::isfinite(std::hypot(n[0],n[1],n[2]))&&std::hypot(n[0],n[1],n[2])>0,
        "mesh_export: zero/nonfinite actual normal");
      require(parent.normals.emplace(key->second,n).second,"mesh_export: repeated physical vertex within polygon");
      parent.boundary.push_back(key->second);points.push_back(xyz[key->second]);
    }
    parent.split=edges.GetSize()==4;
    if(parent.split){require(rectangle(points),"mesh_export: unsupported nonrectangular directed polygon");++output.rectangle_count;}
    else ++output.original_triangles;
    parents.push_back(std::move(parent));
  }
  require(parents.size()<ULONG(-1),"mesh_export: parent tag count exceeds native slots");
  RestoreParents restoration(poly,parents);
  for(ULONG i=0;i<original.GetSize();++i){
    original[i]->m_lUserLong1=i+1;original[i]->m_lUserLong2=0;original[i]->m_lSmoothGroup=0;
  }
  for(ULONG i=0;i<original.GetSize();++i)if(parents[i].split){
    // SDK IsConvexCorner interprets live source labels as UV geometry. Detach
    // only this raw-XYZ rectangle; exact parent ancestry is held separately.
    original[i]->SetOriginalFace(nullptr,FALSE);SmTArray<SmPolyFace*> created;
    const auto start=std::chrono::steady_clock::now();
    const auto status=original[i]->TriangulatePlanar(created);
    output.official_split_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    ++output.official_split_calls;
    require(status==SM_SUCCESS,"mesh_export: official rectangle triangulation failed; discard complete definition");
  }
  SmTArray<SmPolyVertex*> after;poly.GetPolyVertices(after);
  require(after.GetSize()==vertices.GetSize(),"mesh_export: official triangulator changed physical vertex count");
  std::set<SmPolyVertex*> retained;
  for(ULONG i=0;i<after.GetSize();++i){
    auto key=vertex_ids.find(after[i]);require(key!=vertex_ids.end()&&retained.insert(after[i]).second,
      "mesh_export: official triangulator invented/repeated physical vertex");
    require(same_bytes(point(after[i]),xyz[key->second]),"mesh_export: official triangulator moved actual XYZ");
  }
  SmTArray<SmPolyFace*> polygons;poly.GetPolyFaces(polygons);
  require(uint64_t(polygons.GetSize())<=UINT32_MAX/3,"mesh_export: corner count exceeds native slots");
  std::map<SmPolyFace*,size_t> ancestry;
  std::vector<std::map<Edge,unsigned>> boundaries(parents.size());std::vector<unsigned> children(parents.size(),0);
  output.corners.reserve(size_t(polygons.GetSize())*3);output.triangles.reserve(polygons.GetSize());
  std::set<uint64_t> used;std::vector<uint64_t> coverage(source_faces.GetSize(),0);
  for(ULONG i=0;i<polygons.GetSize();++i){
    auto *face=polygons[i];require(face&&face->m_lUserLong1>0&&face->m_lUserLong1<=parents.size(),
      "mesh_export: official child has unknown parent ancestry");
    const size_t index=face->m_lUserLong1-1;const auto &parent=parents[index];
    require(ancestry.emplace(face,index).second,"mesh_export: repeated final polygon");
    require(face->GetOriginalFace()==(parent.split?nullptr:parent.source),"mesh_export: original polygon label changed unexpectedly");
    auto edges=ring(face);require(edges.GetSize()==3,"mesh_export: official split left a nontriangle");
    Triangle triangle{{},source_ids.at(parent.source)};std::array<uint64_t,3> keys{};
    for(ULONG j=0;j<3;++j){
      auto key=vertex_ids.find(edges[j]->GetStartPolyVertex());require(key!=vertex_ids.end(),"mesh_export: final triangle has foreign vertex");
      auto n=parent.normals.find(key->second);require(n!=parent.normals.end(),"mesh_export: child corner has no original parent normal");
      keys[j]=key->second;used.insert(key->second);triangle.corners[j]=static_cast<uint32_t>(output.corners.size());
      output.corners.push_back({xyz[key->second],n->second,key->second});
    }
    require(keys[0]!=keys[1]&&keys[0]!=keys[2]&&keys[1]!=keys[2],"mesh_export: collapsed physical triangle");
    const auto &a=xyz[keys[0]],&b=xyz[keys[1]],&c=xyz[keys[2]];
    Vec cross={(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]),
      (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]),
      (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])};
    require(finite(cross)&&std::hypot(cross[0],cross[1],cross[2])>0,"mesh_export: zero/nonfinite triangle area");
    for(size_t j=0;j<3;++j)++boundaries[index][{keys[j],keys[(j+1)%3]}];
    ++children[index];++coverage[triangle.face];output.triangles.push_back(triangle);
  }
  for(size_t i=0;i<parents.size();++i){
    const auto &parent=parents[i];auto remaining=boundaries[i];
    require(children[i]==(parent.split?2u:1u),"mesh_export: original polygon child count differs");
    for(size_t j=0;j<parent.boundary.size();++j){
      const Edge edge{parent.boundary[j],parent.boundary[(j+1)%parent.boundary.size()]};
      auto found=remaining.find(edge);require(found!=remaining.end()&&found->second==1,
        "mesh_export: original directed polygon boundary changed");remaining.erase(found);
    }
    if(parent.split){
      require(remaining.size()==2,"mesh_export: rectangle must have one shared diagonal");
      const auto first=remaining.begin();auto opposite=remaining.find({first->first.second,first->first.first});
      require(first->second==1&&opposite!=remaining.end()&&opposite!=first&&opposite->second==1,
        "mesh_export: rectangle internal diagonal does not cancel");
    }else require(remaining.empty(),"mesh_export: untouched triangle boundary changed");
  }
  require(used.size()==vertices.GetSize(),"mesh_export: unused actual physical vertices");
  for(auto count:coverage)require(count>0,"mesh_export: imported NVIDIA face omitted from actual triangles");
  require(restoration.restore(),"mesh_export: unknown parent during metadata restoration");
  // Ancestry survives restoration independently of temporary user-field values.
  for(const auto &entry:ancestry)
    require(entry.first->GetOriginalFace()==parents[entry.second].source,"mesh_export: original source label was not restored");
  return output;
}
