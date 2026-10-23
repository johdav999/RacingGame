#include "static_art.hpp"
#include "street_art_contract.hpp"
#include "city_kit_contract.hpp"
#include "building_art_contract.hpp"
#include "track_art_contract.hpp"
#include "plaza_art_contract.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_detail.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace engine;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
using Corner=std::array<std::uint32_t,12>;
using Triangle=std::array<Corner,3>;
// Oriented triangle surfaces must survive reordering, including all UV/normal/
// tangent values. Cyclic corner rotation is allowed, reversed winding is not.
std::vector<Triangle> surfaces(const Mesh& mesh){
 std::vector<Triangle> result;
 for(std::size_t i=0;i<mesh.indices.size();i+=3){
  Triangle t;
  for(int n=0;n<3;n++){
   const auto& v=mesh.vertices[mesh.indices[i+n]];
   const std::array<float,12> values{v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,v.uv.x,v.uv.y,v.tangent[0],v.tangent[1],v.tangent[2],v.tangent[3]};
   for(int k=0;k<12;k++)t[n][k]=std::bit_cast<std::uint32_t>(values[k]);
  }
  if(t[1]<t[0]&&t[1]<t[2])std::rotate(t.begin(),t.begin()+1,t.end());
  else if(t[2]<t[0]&&t[2]<t[1])std::rotate(t.begin(),t.begin()+2,t.end());
  result.push_back(t);
 }
 std::sort(result.begin(),result.end());return result;
}
int main(){
 std::uint64_t sourceVertices=0,optimizedVertices=0,triangles=0;
 std::array<std::uint64_t,3> lodTriangles{};unsigned count=0;
 auto check=[&](const char* group,const auto& models){for(const auto& model:models){
  auto file=std::string(RACING_ASSET_DIR)+"/models/"+group+"/"+model.name+"/"+model.name+".glb";
  GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=16*1024*1024;
  auto source=GlbImporter::loadFile(file,options);auto prepared=racing::art::prepareStatic(*source);
  require(source->lods.empty(),"Preparation must not mutate the source");
  require(prepared->indices.size()==source->indices.size(),"Full-detail face count must be preserved");
  require(prepared->vertices.size()<=source->vertices.size(),"Optimization must not expand vertices");
  require(prepared->materials==source->materials,"Materials/maps must be shared");
  require(surfaces(*prepared)==surfaces(*source),"Full-detail surfaces must remain bit-for-bit identical");
  require(prepared->lods.size()==3,"Static models must have three shared derived LODs");
  auto previous=prepared->indices.size();
  for(unsigned i=0;i<3;i++){
   const auto& lod=*prepared->lods[i].mesh;
   require(lod.indices.size()<=previous,"LOD triangle counts must be monotonic");previous=lod.indices.size();
   require(lod.materials==source->materials&&lod.parts.size()==source->parts.size(),"LOD material partitions must be stable");
   require(selectMeshLod(*prepared,prepared->lods[i].maxPixels*.8f,0)>=i+1,"Screen size must select derived geometry");
   for(auto index:lod.indices)require(index<lod.vertices.size(),"LOD index must be valid");
   lodTriangles[i]+=lod.indices.size()/3;
  }
  require(selectMeshLod(*prepared,10000,3)==0,"Close views must restore original detail");
  sourceVertices+=source->vertices.size();optimizedVertices+=prepared->vertices.size();triangles+=prepared->indices.size()/3;++count;
  std::cout<<model.name<<": "<<prepared->indices.size()/3;
  for(const auto& lod:prepared->lods)std::cout<<" / "<<lod.mesh->indices.size()/3;
  std::cout<<" triangles\n";
 }};
 check("street-kit",racing::streetkit::models);check("city-kit",racing::citykit::models);
 check("buildings",racing::buildingart::models);check("track-kit",racing::trackkit::models);check("plaza",racing::plaza::models);
 require(count==48&&lodTriangles[2]<triangles/3,"Static scene must have substantial distant reduction");
 std::cout<<"TOTAL "<<count<<" models; vertices "<<sourceVertices<<" -> "<<optimizedVertices<<"; triangles "<<triangles;
 for(auto value:lodTriangles)std::cout<<" / "<<value;
 std::cout<<"; exact near surfaces, shared maps and automatic LOD selection passed\n";
}
