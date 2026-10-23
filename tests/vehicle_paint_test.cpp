#include "starter_scene.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace racing;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> read(const char* path){std::ifstream f(std::string(RACING_ASSET_DIR)+path,std::ios::binary);check(bool(f),"asset missing");std::vector<char> b((std::istreambuf_iterator<char>(f)),{});return {reinterpret_cast<const std::byte*>(b.data()),reinterpret_cast<const std::byte*>(b.data()+b.size())};}
int main(){try {
 Progression legacy;legacy.credits=1234;auto old=legacy.encode();old="NA3 "+old.substr(4,old.find(" / ")-4);
 auto migrated=Progression::decode(old);check(migrated&&migrated->credits==1234&&migrated->paint==legacy.paint,"NA3 migration");
 check(!Progression::decode(old+" / 0 1 2 3"),"legacy rejects trailing data");
 StarterScene scene;
 scene.installPlayerCar(read("/models/cybercar-clean-detail/cybercar.glb"));
 const char* paths[]={"","/models/skimmer/skimmer.glb","/models/bastion/bastion.glb","/models/vector/vector.glb"};
 for(int c=1;c<4;++c)scene.installVehicleArt(static_cast<Chassis>(c),read(paths[c]));
 for(int c=0;c<4;++c) {
  scene.selectChassis(static_cast<Chassis>(c));
  auto meshes=scene.player().getComponents<engine::MeshComponent>();auto* body=*std::find_if(meshes.begin(),meshes.end(),[](auto* m){return m->name=="chassis"||m->name=="cybercar_chassis";});
  auto original=body->resolvedMaterial(0).baseColorTexture;
  check(original!=nullptr,"source body texture");
  check(scene.setPaint(c,0xe12540+c),"paint accepted in garage");
  auto painted=body->resolvedMaterial(0).baseColorTexture;
  check(painted!=original,"player texture override");check(body->mesh->materials[0]->baseColorTexture==original,"shared source immutable");
  PaintSurface surface;surface.prepare(*original);std::size_t changed=0,masked=0,protectedCount=0;
  for(std::size_t i=0;i<surface.mask.size();++i) {
   bool diff=false;for(int n=0;n<4;++n)diff|=painted->pixels[i*4+n]!=surface.base->pixels[i*4+n];
   changed+=diff;masked+=surface.mask[i]>0;
   if(!surface.mask[i]){check(!diff,"unmasked trim retained exactly");++protectedCount;}
  }
  check(changed>100&&masked>100&&protectedCount>100,"mask has both paint and protected texels");
  check(!scene.setPaint((c+1)%4,1)&&!scene.setPaint(c,-1)&&!scene.setPaint(c,0x1000001),"reject invalid paint commands");
 }
 auto saved=scene.progression().encode();auto decoded=Progression::decode(saved);
 check(decoded&&decoded->paint==scene.progression().paint,"four paint colors round trip");
 check(!scene.restoreProgression(saved+" extra")&&scene.progression().encode()==saved,"corrupt restore preserves career");
 check(!Progression::decode(saved.substr(0,saved.rfind(' '))+" 16777217"),"out of range save rejected");
 check(scene.restoreProgression(saved),"restore colors");
 scene.selectChassis(Chassis::Vektor);check(scene.setPaint(0,Progression::originalPaint),"reset original");
 auto meshes=scene.player().getComponents<engine::MeshComponent>();auto* body=*std::find_if(meshes.begin(),meshes.end(),[](auto* m){return m->name=="cybercar_chassis";});check(body->resolvedMaterial(0).baseColorTexture==body->mesh->materials[0]->baseColorTexture,"original restores authored resolution");
 scene.start();check(!scene.setPaint(0,123),"race locks customization");
 std::cout<<"Paint masks, immutable assets, four chassis, migration, corrupt preservation and race lock passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
