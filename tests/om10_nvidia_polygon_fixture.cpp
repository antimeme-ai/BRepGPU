// Literal SDK polygon input tests the production extraction boundary, not CAD truth.
#include <SmApiGeneral.h>
#include <SmApiPrimitives.h>
#include <SmBrep.h>
#include <SmPoly.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "raw_mesh.hpp"

namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Vec normal_input(unsigned face,unsigned corner){return {double(face)+.125,double(corner)+.25,1.5};}
Vec xyz(SmPolyVertex *v){const auto &p=v->GetPoint();return {p[0],p[1],p[2]};}
SmPoint3d point(const Vec &p){return {p[0],p[1],p[2]};}
struct Fixture {
  std::unique_ptr<SmBrep> source;
  std::unique_ptr<SmPolyBrep> poly;
  std::vector<SmFace*> labels;
  std::map<SmFace*,uint64_t> ids;
  std::vector<SmPolyVertex*> quad;
  std::map<std::pair<uint64_t,SmPolyVertex*>,Vec> normals;
  Fixture(unsigned axis,unsigned shift,bool reversed,const std::string &kind){
    auto *context=SmApiGetOrCreateContext();SmBrep *raw=nullptr;
    check(SmApiCreateBox(SmVector3d(0,0,0),1,1,1,raw)==SM_SUCCESS&&raw,"literal source label fixture failed");
    source.reset(raw);SmTArray<SmFace*> fs;source->GetFaces(fs);check(fs.GetSize()==6,"literal source label inventory differs");
    for(unsigned i=0;i<6;++i){labels.push_back(fs[i]);ids.emplace(fs[i],i);}
    poly.reset(new (*context) SmPolyBrep(1.e-9));
    const unsigned r=(axis+1)%3,s=(axis+2)%3;Vec a{};a[axis]=.125;a[r]=.25;a[s]=.5;
    Vec b=a;b[axis]=2.125;Vec c=b;c[r]=3.25;c[s]=4.5;Vec d=a;d[r]=3.25;d[s]=4.5;
    if(kind=="nonrectangle")c[r]+=.125;
    std::vector<Vec> coordinates{a,b,c,d};
    for(const auto &p:coordinates)quad.push_back(new (poly.get()) SmPolyVertex(point(p),1.e-9));
    std::vector<unsigned> order=kind=="crossed"?std::vector<unsigned>{0,2,1,3}:std::vector<unsigned>{0,1,2,3};
    if(reversed)std::reverse(order.begin(),order.end());
    std::rotate(order.begin(),order.begin()+shift%4,order.end());
    std::vector<SmPolyVertex*> ordered;for(auto i:order)ordered.push_back(quad[i]);
    add_face(0,ordered);
    quad=ordered;
    // Untouched triangles share the first rectangle vertex, with independent
    // deliberately different parent normals/user fields and literal other points.
    for(unsigned i=1;i<6;++i){
      if(kind=="sdk_noncompletion"&&i==1){
        Vec t{},u{},v{},w{};u[axis]=std::ldexp(1.,-900);v=u;v[r]=1.;v[s]=1.;w[r]=1.;w[s]=1.;
        std::vector<SmPolyVertex*> tiny;
        for(const auto &p:{t,u,v,w})tiny.push_back(new (poly.get()) SmPolyVertex(point(p),1.e-9));
        add_face(i,tiny);continue;
      }
      Vec p=a,q=a;p[r]+=10+i;q[s]+=20+i;
      auto *pv=new (poly.get()) SmPolyVertex(point(p),1.e-9);
      auto *qv=new (poly.get()) SmPolyVertex(point(q),1.e-9);
      add_face(i,{quad[0],pv,qv});
    }
  }
  void add_face(unsigned index,const std::vector<SmPolyVertex*> &vertices){
    auto *face=new (poly.get()) SmPolyFace(1.e-9,poly.get());
    face->SetOriginalFace(labels[index],FALSE);face->m_lUserLong1=100+index;face->m_lUserLong2=200+index;face->m_lSmoothGroup=300+index;
    SmPolyLoop *loop=nullptr;check(face->StartPolyEdgeLoop(loop)==SM_SUCCESS&&loop,"literal loop creation failed");
    for(unsigned i=0;i<vertices.size();++i){
      auto *a=vertices[i],*b=vertices[(i+1)%vertices.size()];SmPolyEdge *edge=nullptr;
      check(loop->AddPolyEdge(1.e-9,a->GetPoint(),b->GetPoint(),a,b,poly.get(),edge)==SM_SUCCESS&&edge,
        "literal directed edge creation failed");
      auto *aux=new (*SmApiGetOrCreateContext()) SmPolyVertAuxData;
      const auto n=normal_input(index,i);aux->m_vNormal=SmVector3d(n[0],n[1],n[2]);edge->SetAuxData(aux);
      normals.emplace(std::make_pair(index,a),n);
    }
    check(loop->FinishPolyEdgeLoop(poly.get())==SM_SUCCESS,"literal directed ring closure failed");
  }
  void qualify(){
    SmTArray<SmPolyVertex*> before;poly->GetPolyVertices(before);
    std::vector<Vec> original;std::map<SmPolyVertex*,uint64_t> keys;
    for(unsigned i=0;i<before.GetSize();++i){original.push_back(xyz(before[i]));keys.emplace(before[i],i);}
    check(before.GetSize()==14,"literal physical inventory differs");
    const Mesh mesh=extract(*source,*poly);
    check(mesh.original_polygons==6&&mesh.original_triangles==5&&mesh.rectangle_count==1&&mesh.official_split_calls==1,
      "literal official split counters differ");
    check(mesh.physical_count==14&&mesh.triangles.size()==7&&mesh.corners.size()==21,"literal two-triangle extraction inventory differs");
    std::map<std::pair<uint64_t,uint64_t>,unsigned> directed;unsigned rectangles=0;
    std::map<uint64_t,unsigned> source_counts;
    for(const auto &t:mesh.triangles){
      check(t.face<6,"literal source label lost");++source_counts[t.face];
      for(unsigned j=0;j<3;++j){
        const auto &corner=mesh.corners[t.corners[j]];check(corner.physical<original.size(),"literal physical key invented");
        check(corner.point==original[corner.physical],"literal XYZ moved");
        const auto expected=normals.at({t.face,before[corner.physical]});
        check(std::memcmp(corner.normal.data(),expected.data(),sizeof(Vec))==0,"literal parent corner normal changed");
        if(t.face==0)++directed[{corner.physical,mesh.corners[t.corners[(j+1)%3]].physical}];
      }
      if(t.face==0)++rectangles;
    }
    check(rectangles==2&&source_counts[0]==2,"literal rectangle triangle count differs");
    for(unsigned i=1;i<6;++i)check(source_counts[i]==1,"untouched triangle changed count");
    for(unsigned i=0;i<4;++i){auto edge=std::make_pair(keys.at(quad[i]),keys.at(quad[(i+1)%4]));
      check(directed[edge]==1,"literal original directed boundary lost");directed.erase(edge);}
    check(directed.size()==2,"literal shared diagonal inventory differs");
    auto first=directed.begin();check(first->second==1&&directed[{first->first.second,first->first.first}]==1,
      "literal internal diagonal does not cancel");
    SmTArray<SmPolyVertex*> after;poly->GetPolyVertices(after);check(after.GetSize()==before.GetSize(),"literal SDK added vertices");
    for(unsigned i=0;i<after.GetSize();++i)check(keys.count(after[i])&&xyz(after[i])==original[keys.at(after[i])],"literal physical identity changed");
    SmTArray<SmPolyFace*> fs;poly->GetPolyFaces(fs);
    for(unsigned i=0;i<fs.GetSize();++i){const auto id=ids.at(fs[i]->GetOriginalFace());
      check(fs[i]->m_lUserLong1==100+id&&fs[i]->m_lUserLong2==200+id&&fs[i]->m_lSmoothGroup==300+id,"literal parent user data not restored");}
  }
};
}
int main(int argc,char **argv){try{
  check(argc==2,"fixture mode required");SmApiCreateContext();const std::string mode=argv[1];
  if(mode=="admit"){
    for(unsigned axis=0;axis<3;++axis)for(unsigned shift=0;shift<4;++shift)for(bool reversed:{false,true}){
      Fixture f(axis,shift,reversed,"rectangle");f.qualify();}
    std::cout<<"PASS 24 literal any-axis directed rectangles with untouched shared-vertex siblings\n";
  }else if(mode=="reject"){
    for(const std::string kind:{"crossed","nonrectangle"}){Fixture f(1,0,false,kind);bool rejected=false;
      try{extract(*f.source,*f.poly);}catch(const std::exception &){rejected=true;}check(rejected,"inadmissible literal polygon accepted");}
    std::cout<<"PASS crossed and nonrectangular literal rings rejected\n";
  }else if(mode=="restore_failure"){
    Fixture f(1,0,false,"sdk_noncompletion");bool rejected=false;
    try{extract(*f.source,*f.poly);}catch(const std::exception &error){
      rejected=true;std::cout<<"Actual SDK noncompletion: "<<error.what()<<'\n';}
    check(rejected,"underflow-scale SDK rectangle unexpectedly completed");
    SmTArray<SmPolyFace*> faces;f.poly->GetPolyFaces(faces);std::map<uint64_t,unsigned> counts;
    for(unsigned i=0;i<faces.GetSize();++i){
      const auto id=f.ids.at(faces[i]->GetOriginalFace());++counts[id];
      check(faces[i]->m_lUserLong1==100+id&&faces[i]->m_lUserLong2==200+id&&faces[i]->m_lSmoothGroup==300+id,
        "metadata not restored after actual SDK noncompletion");}
    check(counts[0]==2&&counts[1]==1,"literal failure did not follow an earlier completed official split");
    for(unsigned i=2;i<6;++i)check(counts[i]==1,"untouched sibling changed during failed mutation");
    std::cout<<"PASS valid parent/children restored before failed mesh disposal\n";
  }else throw std::runtime_error("unknown fixture mode");
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
