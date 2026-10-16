#include "../native/om10/types.hpp"
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
void check(const bgom10::Api *old_api,const bgom10::Api *api,int32_t device) {
  using namespace bgom10;
  require(old_api&&api&&old_api->version==1&&api->version==2,"explicit display ABI versions");
  Carrier carrier{};carrier.id=1;carrier.tag=1;carrier.x[0]=carrier.y[1]=carrier.z[2]=1;
  Vertex vertex{};vertex.xyz[0]=2;vertex.xyz[1]=3;vertex.xyz[2]=4;
  vertex.flags=STORED_XYZ|COFACTOR_NORMAL;
  SceneView scene{};scene.carriers=&carrier;scene.ncarriers=1;scene.vertices=&vertex;scene.nvertices=1;
  Error error{};Times times{};void *token=nullptr;
  require(!old_api->create(&scene,1,device,&token,&error,&times)&&token==nullptr,
          "legacy factory silently consumed new normal flags");
  require(api->create(&scene,1,device,&token,&error,&times)&&token,"new profile session create");
  Jet jet{};require(api->evaluate(token,&jet,&error,&times),"new profile source evaluate");
  require(jet.used[0]==2&&jet.used[1]==3&&jet.used[2]==4&&jet.normal[2]==1&&
            jet.flags==COFACTOR_INSTANCE_NORMAL,"stored point/normal semantics lost at ABI");
  require(api->close(token,&error,&times),"new profile session close");token=nullptr;
  vertex.flags=0;
  require(!api->create(&scene,1,device,&token,&error,&times)&&token==nullptr,
          "new profile accepted vertex without stored XYZ/cofactor declaration");
}
}
int main(int argc,char **argv) {
  void *plugin=nullptr;
  try {
    check(bgom10::cpu_api(),bgom10::cpu_api_v2(),0);
    if(argc>1) {
      plugin=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);require(plugin,"cannot load display plugin");
      const auto old_factory=reinterpret_cast<const bgom10::Api*(*)()>(dlsym(plugin,"brepgpu_om10_cuda_v1"));
      const auto factory=reinterpret_cast<const bgom10::Api*(*)()>(dlsym(plugin,"brepgpu_om10_cuda_v2"));
      require(old_factory&&factory,"both explicit display factories required");
      check(old_factory(),factory(),argc>2?std::stoi(argv[2]):0);
      dlclose(plugin);plugin=nullptr;
    }
    std::cout<<"PASS\texplicit stored-source display ABI\n";return 0;
  }catch(const std::exception &error) {
    if(plugin)dlclose(plugin);std::cerr<<error.what()<<'\n';return 1;
  }
}
