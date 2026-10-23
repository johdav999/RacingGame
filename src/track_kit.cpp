#include "track_kit.hpp"
#include "static_art.hpp"
#include "gantry_mount.hpp"
#include "engine/mesh_component.hpp"
#include <algorithm>
#include <stdexcept>

namespace racing::trackkit {
using namespace engine;
namespace {
float lengthYaw(Vec3 delta) {return std::atan2(-delta.z,delta.x);}
float frontYaw(Vec3 front) {return std::atan2(front.x,front.z);}
MeshComponent& glow(Actor& actor,const char* name,Vec3 position,Vec3 size,Vec3 color) {
    auto& m=actor.addComponent<MeshComponent>(name,Mesh::cube());
    m.transform.position=position;m.transform.scale=size;m.color=color;m.castsShadow=false;
    m.allowInstanceReordering=true;MaterialInstance material;material.shader=MaterialShader::Unlit;
    m.setMaterial(0,material);return m;
}
void bar(Actor& a,float x1,float y1,float x2,float y2,float z,Vec3 color,float thickness=.075f) {
    float dx=x2-x1,dy=y2-y1;
    auto& m=glow(a,"Road sign marking",{(x1+x2)*.5f,(y1+y2)*.5f,z},{std::hypot(dx,dy),thickness,.018f},color);
    m.transform.rotation.z=std::atan2(dy,dx);
}
void number(Actor& a,int value,float y,float z) {
    const std::string label=std::to_string(value);float step=.2f;
    for(unsigned i=0;i<label.size();++i) {
        float x=(float(i)-(label.size()-1)*.5f)*step;
        unsigned mask=label[i]=='0'?0x3f:label[i]=='1'?0x06:0x6d; // Seven-segment 0, 1, 5.
        constexpr float segment[7][4]={{-.065f,.17f,.065f,.17f},{.065f,.17f,.065f,0},{.065f,0,.065f,-.17f},
            {-.065f,-.17f,.065f,-.17f},{-.065f,0,-.065f,-.17f},{-.065f,.17f,-.065f,0},{-.065f,0,.065f,0}};
        for(unsigned j=0;j<7;++j)if(mask&(1u<<j))bar(a,x+segment[j][0],y+segment[j][1],x+segment[j][2],y+segment[j][3],z,{1,.85f,.48f},.035f);
    }
}
}
bool Kit::ready() const {return std::all_of(loaded.begin(),loaded.end(),[](bool b){return b;});}
void Kit::plan(const Circuit& track,const std::vector<buildingart::Placement>& buildings) {
    instances.clear();
    auto add=[&](unsigned model,Vec3 p,float yaw=0,Vec3 scale=Vec3{1,1,1},int marking=0) {
        instances.push_back({model,p,yaw,scale,marking});
    };
    for(int i=0;i<Circuit::samples;i+=2)for(int side:{-1,1}) {
        auto segment=[&](float offset,unsigned model,float width,float y) {
            auto a=track.point(i)+track.right(i)*(side*offset);
            auto b=track.point(i+2)+track.right(i+2)*(side*offset);
            auto center=(a+b)*.5f;center.y=y;
            add(model,center,lengthYaw(b-a)+(side<0?pi:0),{(magnitude(b-a)+.07f)/width,1,1});
        };
        segment(11.3f,0,4,0); // Match the existing continuous physics envelope.
        segment(12.95f,9,4,.025f);
        segment(11.85f,10,4,.025f);
        if(i%16==4)add(12,track.point(i)+track.right(i)*(side*12.15f)+Vec3{0,.13f,0},lengthYaw(track.tangent(i)));
    }
    for(int g=0;g<Circuit::gates;++g)add(g==0?3:4,track.gate(g),frontYaw(track.gateForward(g)*-1.f));
    for(int i=4;i<Circuit::samples;i+=12) {
        int side=(i/12)%2?-1:1;auto inward=track.right(i)*float(-side);
        add(5,track.point(i)+track.right(i)*(side*13.2f)+Vec3{0,.18f,0},lengthYaw(inward));
    }
    // Paired corner slabs connect the repeated sidewalk to building approaches.
    for(const auto& building:buildings) {
        Vec3 front{std::cos(building.yaw),0,-std::sin(building.yaw)};
        Vec3 tangent{front.z,0,-front.x};
        const auto d=buildingart::models[building.model].dimensions;
        auto entry=building.position+front*(d.x*.5f+.2f);
        for(int side:{-1,1}) {
            auto p=entry+tangent*(side*1.2f);p.y=.025f;
            add(11,p,frontYaw(front)+(side<0?pi*.5f:0));
            // Wall-mounted lamp is seated at the building's frontmost surface.
            p=entry+tangent*(side*1.8f);p.y=3.2f;add(6,p,frontYaw(front));
        }
    }
    // Corner and end modules delimit four small service lay-bys behind the race barrier.
    for(int sample:{28,68,124,164}) {
        auto t=track.tangent(sample),r=track.right(sample),p=track.point(sample)+r*16.1f;
        add(1,p,lengthYaw(t));add(2,p+t*2.5f,lengthYaw(t));
    }
    for(int apex:{0,96}) {
        for(int offset:{-5,0,5}) {
            int i=apex+offset;auto t=track.tangent(i);
            int turn=dot(track.tangent(i+4)-track.tangent(i-4),track.right(i))>0?1:-1;
            add(7,track.point(i)+track.right(i)*13.f+Vec3{0,.18f,0},frontYaw(t*-1.f),{1,1,1},turn);
        }
        for(int distance:{100,50}) {
            float remaining=float(distance);int i=apex;
            while(remaining>magnitude(track.point(i)-track.point(i-1))) {
                remaining-=magnitude(track.point(i)-track.point(i-1));--i;
            }
            auto forward=normalized(track.point(i)-track.point(i-1));
            auto p=track.point(i)-forward*remaining+Vec3{forward.z,0,-forward.x}*12.7f;
            p.y=.18f;add(8,p,frontYaw(forward*-1.f),{1,1,1},distance);
        }
    }
}
void Kit::install(Scene& scene,unsigned model,std::span<const std::byte> bytes) {
    if(model>=loaded.size()||loaded[model])throw std::logic_error("Invalid or installed track-kit model");
    const auto& expected=models[model];
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=16*1024*1024;
    auto mesh=GlbImporter::loadMemory(bytes,options);const auto& bounds=mesh->localBounds();
    if(!mesh->warnings.empty()||mesh->parts.size()!=1||mesh->materials.size()!=1||
       mesh->indices.size()!=expected.triangles*3||!bounds.valid||
       magnitude(bounds.maximum-bounds.minimum-expected.dimensions)>.025f||std::abs(bounds.minimum.y)>.01f)
        throw std::runtime_error(std::string("Track-kit asset contract mismatch: ")+expected.name+
            " parts="+std::to_string(mesh->parts.size())+" materials="+std::to_string(mesh->materials.size())+
            " triangles="+std::to_string(mesh->indices.size()/3)+" warnings="+std::to_string(mesh->warnings.size())+
            " size-error="+std::to_string(magnitude(bounds.maximum-bounds.minimum-expected.dimensions)));
    const float mountingTop=(model==3||model==4)?gantryBeamTop(*mesh):0;
    mesh=art::prepareStatic(*mesh);
    std::vector<std::unique_ptr<Actor>> staged;std::vector<Vec3> newEmitters;
    for(const auto& p:instances)if(p.model==model) {
        auto actor=std::make_unique<Actor>(expected.name);
        actor->transform.position=p.position;actor->transform.rotation.y=p.yaw;
        auto& visual=actor->addComponent<MeshComponent>("Track kit model",mesh);
        visual.transform.scale=p.scale;visual.allowInstanceReordering=true;
        if(model==5||model==6) {
            Vec3 emitter=model==5?Vec3{.85f,6.8f,0}:Vec3{0,.35f,.2f};
            newEmitters.push_back(transformPoint(actor->transform.matrix(),emitter));
            if(model==5)glow(*actor,"Streetlight diffuser",emitter,{.5f,.025f,.22f},{1,.83f,.54f});
        }
        if(model==7)for(float x:{-.6f,0.f,.6f}) {
            float direction=float(p.marking),y=1.75f,z=.325f;
            bar(*actor,x-direction*.18f,y+.28f,x+direction*.16f,y,z,{1,.65f,.12f});
            bar(*actor,x+direction*.16f,y,x-direction*.18f,y-.28f,z,{1,.65f,.12f});
        }
        if(model==8) {
            glow(*actor,"Braking board contrast panel",{0,1.5f,.3f},{.7f,.65f,.018f},{.008f,.012f,.02f});
            number(*actor,p.marking,1.5f,.325f);
        }
        staged.push_back(std::move(actor));
    }
    for(auto& actor:staged)scene.adoptChild(std::move(actor));
    emitters.insert(emitters.end(),newEmitters.begin(),newEmitters.end());
    if(model==3||model==4)beamTops[model-3]=mountingTop;
    loaded[model]=true;scene.synchronize();
}
void Kit::illuminate(Scene& scene,Vec3 viewer) {
    if(emitters.empty())return;
    if(!lights[0]) {
        auto& actor=scene.spawn("Nearest street lighting");
        for(auto& light:lights) {
            light=&actor.addComponent<LightComponent>("Warm street lamp",LightType::Point);
            light->color={1,.75f,.46f};light->intensity=5;light->range=19;
        }
        scene.synchronize();
    }
    std::vector<Vec3> nearest=emitters;
    std::partial_sort(nearest.begin(),nearest.begin()+std::min(nearest.size(),lights.size()),nearest.end(),
        [&](Vec3 a,Vec3 b){return dot(a-viewer,a-viewer)<dot(b-viewer,b-viewer);});
    for(unsigned i=0;i<lights.size();++i) {
        lights[i]->intensity=i<nearest.size()?5.f:0.f;
        if(i<nearest.size())lights[i]->transform.position=nearest[i];
    }
}
}
