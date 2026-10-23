#include "market_plaza.hpp"
#include "engine/mesh_component.hpp"
#include "engine/collider_component.hpp"
#include "engine/water.hpp"
#include <fstream>
#include <iostream>
using namespace engine;
using namespace racing;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
 Scene scene;Circuit track;plaza::Kit kit;
 std::array<std::byte,4> invalid{};bool rejected=false;
 try{kit.install(scene,track,0,invalid);}catch(const std::exception&){rejected=true;}
 require(rejected&&!kit.ready()&&scene.children().empty(),"Malformed import must be atomic");
 for(unsigned id=0;id<plaza::models.size();++id){
  std::string name=plaza::models[id].name;
  std::ifstream file(std::string(RACING_ASSET_DIR)+"/models/plaza/"+name+"/"+name+".glb",std::ios::binary|std::ios::ate);
  require(bool(file),"Missing plaza asset");std::vector<std::byte> bytes(static_cast<size_t>(file.tellg()));file.seekg(0);
  file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());kit.install(scene,track,id,bytes);
  rejected=false;try{kit.install(scene,track,id,bytes);}catch(const std::logic_error&){rejected=true;}
  require(rejected,"Duplicate import rejected");
 }
 require(kit.ready(),"Both modules must load");
 unsigned paving=0,drains=0,damp=0;const Texture* pavingMap=nullptr;
 for(auto& node:scene.children()){
  auto* actor=dynamic_cast<Actor*>(node.get());require(actor&&!actor->getComponent<ColliderComponent>(),"Scenery must not change barrier collision");
  for(auto* visual:actor->getComponents<MeshComponent>()){
   if(visual->name=="Residual rain / neon reflection"){
    ++damp;require(std::abs(visual->transform.position.y-plaza::surfaceHeight-.006f)<.001f,"Damp patches remain above the paving");
    require(plaza::contains(track,visual->transform.position),"Damp remains in the plaza");continue;
   }
   if(visual->name=="Dry stone paving"){
    ++paving;auto* map=visual->mesh->materials[0]->baseColorTexture.get();
    require(map&&(!pavingMap||pavingMap==map),"Paving shares its texture maps");pavingMap=map;
   }else if(visual->name=="Continuous grated drain")++drains;
   for(const auto& v:visual->mesh->vertices){
    require(track.nearest(v.position).distance>14.f,"Plaza must clear both the racing envelope and existing sidewalk");
    require(v.position.y<=plaza::surfaceHeight+.001f,"Paving and drain must remain low and level");
    require(std::isfinite(v.normal.x)&&std::abs(magnitude(v.normal)-1)<.01f,"Conformed normals must remain normalized");
   }
  }
 }
 require(paving==156&&drains==26&&damp==6,"Expected bounded plaza coverage and sparse damp patches");
 require(!waterPlanarReflectionRequired(scene),"Small damp patches must not request a city reflection pass");
 std::cout<<"Plaza: 156 paving modules, 26 drains, 6 damp patches, shared maps, road clearance and atomic imports passed\n";
}
