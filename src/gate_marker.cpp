#include "gate_marker.hpp"
#include "engine/mesh_component.hpp"
#include <stdexcept>

namespace racing {
using namespace engine;
void GateMarker::install(Scene& scene,std::span<const std::byte> bytes) {
    if(root)throw std::logic_error("Next-gate beacon already installed");
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=8*1024*1024;
    auto mesh=GlbImporter::loadMemory(bytes,options);
    const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->parts.size()!=2||mesh->materials.size()!=2||mesh->indices.size()!=gateart::triangles*3||
       !bounds.valid||magnitude(bounds.maximum-bounds.minimum-gateart::dimensions)>.025f||
       std::abs(bounds.minimum.y)>.01f)
        throw std::runtime_error("Next-gate beacon asset contract mismatch");
    auto staged=std::make_unique<Actor>("Next gate beacon");
    staged->enabled=false;
    auto& visual=staged->addComponent<MeshComponent>("Illuminated next-gate sign",mesh);
    visual.castsShadow=false;
    auto bar=[&](Vec3 p,Vec3 size) {
        auto& m=staged->addComponent<MeshComponent>("Gate edge bracket",Mesh::cube());
        m.transform.position=p;m.transform.scale=size;m.color={1,.56f,.06f};m.castsShadow=false;
        MaterialInstance material;material.shader=MaterialShader::Unlit;m.setMaterial(0,material);
    };
    for(float side:{-1.f,1.f}) {
        bar({side*9.25f,.075f,0},{.16f,.025f,1.8f});
        for(float end:{-1.f,1.f})bar({side*8.7f,.075f,end*.9f},{1.25f,.025f,.16f});
    }
    auto* candidate=staged.get();scene.adoptChild(std::move(staged));root=candidate;sign=&visual;
}
float GateMarker::mountHeight() const {return sign?sign->transform.position.y:0;}
void GateMarker::present(const Circuit& circuit,const RaceProgress& progress,bool racing,float beamTop) {
    if(!root)return;
    root->enabled=racing&&!progress.finished&&beamTop>0;
    // Embed the short support feet into the imported beam's irregular surface.
    // The sign sits above it; road brackets stay at the gate's ground plane.
    sign->transform.position={0,beamTop-.25f,0};
    root->transform.position=circuit.gate(progress.nextGate);
    const auto forward=circuit.gateForward(progress.nextGate);
    root->transform.rotation.y=std::atan2(forward.x,forward.z);
}
}
