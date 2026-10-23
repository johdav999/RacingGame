#include "desert_solar.hpp"
#include "dune_villages.hpp"
#include "static_art.hpp"
#include "desert_surface.hpp"
#include "engine/glb_importer.hpp"
#include "engine/mesh_component.hpp"
#include "engine/collider_component.hpp"
#include "engine/rigid_body_component.hpp"
#include <algorithm>

namespace racing::desert {
using namespace engine;
bool Kit::ready() const {return std::all_of(loaded.begin(),loaded.end(),[](bool v){return v;});}
void Kit::plan(const Circuit& track) {
    instances.clear();
    auto add=[&](unsigned model,Vec3 p,float yaw=0.f,float scale=1.f) {
        // Reserve the whole rotated footprint outside the crash barriers.
        float radius=std::hypot(models[model].dimensions.x,models[model].dimensions.z)*scale*.5f;
        if(track.nearest(p).distance>radius+13)instances.push_back({model,p,yaw,scale});
    };
    // Two dense heliostat fields share just two meshes and their textures.
    for(int row=0;row<7;++row)for(int col=0;col<10;++col) {
        Vec3 p{-56.f+col*11,0,-122.f+row*10};
        unsigned model=(row*10+col)%19==0?1:0;
        add(model,p,.14f+col*.025f);
    }
    add(2,{32,0,10},0);add(2,{-180,0,-80},.45f,.8f);
    add(12,{40,0,-131},0);
    // Dense, overlapping walls at selected bends; gaps reveal deeper formations.
    for(int sample=29;sample<=113;sample+=3)for(int side:{-1,1}) {
        auto t=track.tangent(sample);float yaw=std::atan2(t.x,t.z)+pi*.5f;
        unsigned model=3+unsigned((sample/3+(side==1?1:0))%4);
        float scale=1.35f+.16f*(sample%5);
        auto d=models[model].dimensions*scale;
        auto outward=track.right(sample)*float(side);
        auto p=track.point(sample)+outward*(15.f+d.z*.5f);
        // Sample the complete oriented rectangle, including curved-road edges.
        auto clear=[&] {
            for(int x=-4;x<=4;++x)for(int z=-2;z<=2;++z) {
                auto q=p+t*(x*d.x/8)+outward*(z*d.z/4);
                if(track.nearest(q).distance<13.5f)return false;
            }
            return true;
        };
        for(int move=0;move<30&&!clear();++move)p=p+outward;
        if(clear())instances.push_back({model,p,yaw,scale});
        if(sample%9==2) add(5,track.point(sample)+outward*65.f,yaw+.25f,2.3f+(sample%4)*.2f);
    }
    // Wide arch: the opening is approximately 12m at source scale; 2.2x
    // leaves more than 26m across the 20m road plus its shoulders.
    const int arch=43;auto t=track.tangent(arch);
    instances.push_back({7,track.point(arch)+Vec3{0,-.15f,0},std::atan2(t.x,t.z),2.2f});
    // Raised conduits run alongside the return straight; junctions lead toward
    // the receiver. Blue strips are emissive maps, not hundreds of lights.
    for(int i=130;i<182;i+=2) {
        auto p=track.point(i)+track.right(i)*-17.f;
        auto next=track.point(i+2)+track.right(i+2)*-17.f;
        auto delta=next-p;float yaw=std::atan2(-delta.z,delta.x);
        instances.push_back({11,p, yaw,1});
        p.y=.85f;
        // Separate 4m segments follow the polyline without stretched textures.
        int pieces=std::max(1,int(std::ceil(magnitude(delta)/4)));
        for(int j=0;j<pieces;++j)instances.push_back({8,p+delta*((j+.5f)/pieces),yaw,1});
        if(i==142||i==166)instances.push_back({10,p+track.right(i)*-2.f,yaw,1});
    }
    auto p=track.point(182)+track.right(182)*-17.f;p.y=.85f;
    instances.push_back({9,p,0,1});
    // Height ignores rock Y, so all formations share one stable terrain source.
    for(auto& a:instances)if(a.model>=3&&a.model<=6) {
        // Use the lowest support across the footprint. Center-only grounding
        // suspends the downhill edge of a long shelf above a sloping dune.
        auto d=models[a.model].dimensions*a.scale;
        float support=height(track,a.position.x,a.position.z);
        for(int x=-4;x<=4;++x)for(int z=-2;z<=2;++z) {
            float lx=x*d.x/8,lz=z*d.z/4;
            support=std::min(support,height(track,a.position.x+std::cos(a.yaw)*lx+std::sin(a.yaw)*lz,
                a.position.z-std::sin(a.yaw)*lx+std::cos(a.yaw)*lz));
        }
        a.position.y=support-1.1f*a.scale;
    }
}
float Kit::height(const Circuit& track,float x,float z) const {
    auto smooth=[](float v){v=std::clamp(v,0.f,1.f);return v*v*(3-2*v);};
    float road=track.nearest({x,0,z}).distance;
    float grade=std::max(std::abs(x-5)-80,std::abs(z+90)-50);
    for(const auto& a:instances)if(a.model<3||a.model>=8) {
        auto d=models[a.model].dimensions;
        grade=std::min(grade,std::hypot(x-a.position.x,z-a.position.z)-std::hypot(d.x,d.z)*a.scale*.5f-3);
    }
    // Preserve only actual equipment pads, leaving shallow drifts between rows.
    float clearance=1000;
    for(const auto& a:instances)if(a.model<3||a.model>=8){
        auto d=models[a.model].dimensions*a.scale;
        clearance=std::min(clearance,std::hypot(x-a.position.x,z-a.position.z)-std::hypot(d.x,d.z)*.5f-1);
    }
    for(float cx:{-34.f,40.f})clearance=std::min(clearance,std::max(std::abs(x-cx)-14,std::abs(z+131)-6));
    clearance=std::min(clearance,std::max(std::abs(x-5)-61,std::abs(z+138)-2.5f));
    clearance=std::min(clearance,std::max(std::abs(x-63)-2.5f,std::abs(z+96)-42));
    float drift=std::max(0.f,.35f+.3f*std::sin(x*.28f+z*.19f)+.2f*std::sin(z*.51f-x*.17f))*smooth(clearance/3);
    float ridge=std::sin(x*.024f+z*.013f+std::sin(z*.018f)*1.4f);
    float dune=(5+7*ridge*ridge+3*std::sin(z*.031f+x*.008f))*smooth((road-17)/34)*smooth(grade/22);
    float talus=0;
    for(const auto& a:instances)if(a.model>=3&&a.model<=6) {
        float dx=x-a.position.x,dz=z-a.position.z,c=std::cos(a.yaw),s=std::sin(a.yaw);
        auto d=models[a.model].dimensions*a.scale;
        float edge=std::hypot(std::max(0.f,std::abs(c*dx-s*dz)-d.x*.42f),std::max(0.f,std::abs(s*dx+c*dz)-d.z*.4f));
        talus=std::max(talus,(2.7f+.45f*std::sin(x*.31f+z*.17f))*smooth(1-edge/13));
    }
    return .002f+(dune+talus+drift)*smooth((road-14)/8)*(1-dune::groundBlend(track,x,z));
}
void Kit::dress(Scene& scene,const Circuit& track) const {
    // Match the terrain triangles, not just the analytic surface between vertices.
    auto ground=[&](float x,float z) {
        constexpr float step=3.5f;
        float gx=std::floor(x/step)*step,gz=std::floor(z/step)*step;
        float u=(x-gx)/step,v=(z-gz)/step;
        float a=height(track,gx,gz),b=height(track,gx+step,gz),c=height(track,gx,gz+step);
        if(u+v<=1)return a+(b-a)*u+(c-a)*v;
        float d=height(track,gx+step,gz+step);
        return d+(c-d)*(1-u)+(b-d)*(1-v);
    };
    // Small authored objects are baked by material and spatial cell, avoiding
    // thousands of scene nodes while retaining useful camera culling.
    struct Batch {int material,x,z;std::shared_ptr<Mesh> mesh;};
    std::vector<Batch> batches;
    const std::array<Vec3,12> colors{{{.48f,.28f,.13f},{.32f,.23f,.15f},{.61f,.41f,.22f},
        {.28f,.32f,.31f},{.68f,.64f,.49f},{.055f,.085f,.10f},{.36f,.42f,.21f},{.56f,.72f,.74f},{.67f,.39f,.14f},
        {.48f,.28f,.13f},{.32f,.23f,.15f},{.61f,.41f,.22f}}};
    auto append=[&](const Mesh& source,Vec3 p,Vec3 size,Vec3 rotation,int material) {
        int cx=int(std::floor(p.x/48)),cz=int(std::floor(p.z/48));
        auto it=std::find_if(batches.begin(),batches.end(),[&](const Batch& b){return b.material==material&&b.x==cx&&b.z==cz;});
        if(it==batches.end()){batches.push_back({material,cx,cz,std::make_shared<Mesh>()});it=std::prev(batches.end());}
        auto& dest=*it->mesh;auto offset=static_cast<std::uint32_t>(dest.vertices.size());
        Transform transform;transform.position=p;transform.scale=size;transform.rotation=rotation;
        auto matrix=transform.matrix(),normal=transpose(inverseAffine(matrix));
        for(auto v:source.vertices){v.position=transformPoint(matrix,v.position);v.normal=normalized(transformDirection(normal,v.normal));dest.vertices.push_back(v);}
        for(auto index:source.indices)dest.indices.push_back(offset+index);
    };
    auto cube=Mesh::cube();auto rock=std::make_shared<Mesh>(*Mesh::sphere(7,4));
    for(auto& v:rock->vertices) {
        float n=1+.17f*std::sin(v.position.x*13+v.position.y*7+v.position.z*19);
        v.position=v.position*n;
    }
    for(auto& v:rock->vertices)v.normal={};
    for(std::size_t i=0;i<rock->indices.size();i+=3){auto a=rock->indices[i],b=rock->indices[i+1],c=rock->indices[i+2];
        auto normal=cross(rock->vertices[b].position-rock->vertices[a].position,rock->vertices[c].position-rock->vertices[a].position);
        for(auto index:{a,b,c})rock->vertices[index].normal=rock->vertices[index].normal+normal;
    }
    for(auto& v:rock->vertices)v.normal=normalized(v.normal);
    unsigned random=7143;
    auto rnd=[&]{random=random*1664525u+1013904223u;return float(random>>8)/16777216.f;};
    auto safe=[&](Vec3 p,float radius){return track.nearest(p).distance>13.f+radius;};
    auto stone=[&](Vec3 p,float size,int mat) {
        if(!safe(p,size))return;
        p.y=ground(p.x,p.z)+size*.12f;
        append(*rock,p,{size*1.4f,size*.75f,size},{rnd()*.3f,rnd()*6.28f,rnd()*.25f},9+mat);
    };
    // Rubble follows each rock's toe, with larger fallen slabs nearest the wall.
    for(const auto& a:instances)if(a.model>=3&&a.model<=6) {
        auto d=models[a.model].dimensions*a.scale;
        for(int i=0;i<18;++i) {
            float angle=rnd()*2*pi;
            float rx=(d.x*.42f+2+rnd()*5)*std::cos(angle),rz=(d.z*.44f+2+rnd()*5)*std::sin(angle);
            Vec3 p=a.position+Vec3{std::cos(a.yaw)*rx+std::sin(a.yaw)*rz,0,-std::sin(a.yaw)*rx+std::cos(a.yaw)*rz};
            stone(p,.35f+rnd()*1.6f,i%3);
        }
    }
    // Irregular exposed shelves and gravel along shoulders, never on the road.
    for(int i=0;i<Circuit::samples;++i)for(int side:{-1,1}) {
        auto p=track.point(i)+track.right(i)*(side*(14.5f+rnd()*7));
        for(int j=0;j<3;++j)stone(p+Vec3{rnd()*3,0,rnd()*3},.12f+rnd()*.38f,1);
        if(i%7==0)stone(p,1.2f+rnd(),0);
        if(i%9==0&&safe(p,1)) {
            p.y=ground(p.x,p.z);
            for(int stem=0;stem<5;++stem)append(*rock,p+Vec3{(rnd()-.5f)*.7f,.18f,(rnd()-.5f)*.7f},
                {.16f,.35f+rnd()*.4f,.16f},{rnd()*.7f,rnd()*6,rnd()*.7f},6);
        }
    }
    // Terrain-following translucent patches: wind deposits, gravel and contact
    // shade. Identical geometry/material path on WebGPU and WebGL2.
    auto feather=std::make_shared<Texture>();feather->name="Feathered desert deposits";
    feather->width=feather->height=64;feather->pixels.resize(64*64*4);
    for(int y=0;y<64;++y)for(int x=0;x<64;++x) {
        float r=std::hypot((x-31.5f)/31.5f,(y-31.5f)/31.5f);
        auto n=(y*64+x)*4;for(int c=0;c<3;++c)feather->pixels[n+c]=255;
        feather->pixels[n+3]=std::uint8_t(190*std::pow(std::max(0.f,1-r),.65f));
    }
    auto patch=[&](Vec3 p,float rx,float rz,Vec3 color,float opacity) {
        // Share the terrain grid and diagonal exactly, avoiding intersections
        // where an independently tessellated decal would bridge a talus ridge.
        auto mesh=std::make_shared<Mesh>();constexpr float step=3.5f;
        float startX=std::floor((p.x-rx)/step)*step,startZ=std::floor((p.z-rz)/step)*step;
        int nx=int(std::ceil((p.x+rx-startX)/step)),nz=int(std::ceil((p.z+rz-startZ)/step));
        for(int z=0;z<=nz;++z)for(int x=0;x<=nx;++x) {
            float px=startX+x*step,pz=startZ+z*step;
            mesh->vertices.push_back({{px,ground(px,pz)+.025f,pz},{0,1,0},{std::clamp((px-p.x)/rx*.5f+.5f,0.f,1.f),std::clamp((pz-p.z)/rz*.5f+.5f,0.f,1.f)}});
        }
        for(int z=0;z<nz;++z)for(int x=0;x<nx;++x){auto i=std::uint32_t(z*(nx+1)+x);mesh->indices.insert(mesh->indices.end(),{i,i+nx+1,i+1,i+1,i+nx+1,i+nx+2});}
        mesh->generateTangents();auto& m=scene.spawn("Desert ground deposit").addComponent<MeshComponent>("Terrain contact",mesh);
        m.color=color;m.castsShadow=false;MaterialInstance mat;mat.baseColorTexture=feather;mat.alphaMode=AlphaMode::Blend;mat.opacity=opacity;mat.roughness=1;m.setMaterial(0,mat);
    };
    for(const auto& a:instances)if(a.model<7) {
        auto d=models[a.model].dimensions*a.scale;
        float rx=(std::abs(std::cos(a.yaw))*d.x+std::abs(std::sin(a.yaw))*d.z)*.6f;
        float rz=(std::abs(std::sin(a.yaw))*d.x+std::abs(std::cos(a.yaw))*d.z)*.6f;
        patch(a.position,rx+1,rz+1,{.18f,.12f,.07f},.4f);
        if(a.model==1)for(int i=0;i<9;++i) {
            auto p=a.position+Vec3{(rnd()-.5f)*7,0,(rnd()-.5f)*7};if(!safe(p,1))continue;
            p.y=ground(p.x,p.z)+.08f;append(*rock,p,{.3f+rnd()*.6f,.045f,.2f+rnd()*.4f},{0,rnd()*6,.05f},7);
        }
    }
    for(int i=0;i<Circuit::samples;i+=6) {
        auto p=track.point(i)+track.right(i)*((i%12?1:-1)*19.f);
        if(safe(p,4))patch(p,3.3f,2.2f,i%12?Vec3{.72f,.45f,.22f}:Vec3{.33f,.27f,.19f},.5f);
    }
    // Shared access aisle connects two service compounds to the heliostat rows.
    // The full object radius is reserved against the route before placement.
    auto part=[&](Vec3 p,Vec3 size,int mat,float yaw=0.f) {
        if(!safe(p,std::hypot(size.x,size.z)*.5f))return;
        p.y+=ground(p.x,p.z);append(*cube,p,size,{0,yaw,0},mat);
    };
    for(int x=-52;x<=62;x+=2) {
        part({float(x),.025f,-138},{2.03f,.035f,4.5f},1);
        part({float(x),.08f,-134.8f},{2.03f,.12f,.38f},3); // covered cable trench
    }
    for(int z=-136;z<=-56;z+=2)part({63,.025f,float(z)},{4.5f,.035f,2.03f},1);
    for(Vec3 center: {Vec3{-34,0,-131},Vec3{40,0,-131}}) {
        // Shelter, transformer bank, insulators and access-facing doors.
        auto p=center;
        if(center.x<0){
        part(p+Vec3{0,1.3f,0},{7,2.6f,3.6f},4);
        part(p+Vec3{0,2.7f,0},{7.7f,.22f,4.3f},3);
        part(p+Vec3{-1.8f,1.05f,-1.84f},{1.25f,2.1f,.06f},5);
        part(p+Vec3{1.5f,1.65f,-1.84f},{1.6f,.65f,.06f},7);
        }
        for(int j=0;j<3;++j) {
            auto c=p+Vec3{7.f+j*1.6f,.85f,0};part(c,{1.2f,1.7f,1.6f},3);
            for(int k=0;k<3;++k)part(c+Vec3{-.35f+k*.35f,1.02f,0},{.12f,.35f,.12f},5);
            for(int k=0;k<4;++k)part(c+Vec3{0,-.5f+k*.26f,-.82f},{.85f,.06f,.05f},5);
        }
        // Parked utility pickup: wheels, cab glazing, open cargo bed and beacon.
        auto v=p+Vec3{-7.5f,0,-.5f};
        part(v+Vec3{0,.65f,0},{2.1f,.55f,4.4f},8);
        part(v+Vec3{0,1.25f,-.65f},{1.85f,.85f,1.9f},4);
        part(v+Vec3{0,1.4f,-1.62f},{1.6f,.5f,.06f},5);
        part(v+Vec3{0,.97f,1.2f},{1.7f,.1f,1.4f},5);
        part(v+Vec3{0,1.76f,-.6f},{.4f,.16f,.3f},8);
        for(int side:{-1,1})for(float z:{-1.35f,1.35f})part(v+Vec3{side*1.05f,.39f,z},{.32f,.72f,.72f},5);
        // Fence has an entrance toward the maintenance aisle.
        for(int x=-11;x<=11;x+=2)for(float z:{-3.f,3.f}) {
            if(z<0&&x>-4&&x<4)continue;
            part(p+Vec3{float(x),.85f,z},{.07f,1.7f,.07f},3);
            for(float y:{.5f,1.1f,1.65f})part(p+Vec3{float(x)+.9f,y,z},{1.8f,.035f,.035f},3);
        }
        patch(p+Vec3{1,0,1},12,5,{.22f,.17f,.11f},.25f);
    }
    auto stoneMaterial=surfaceMaterial(true);
    for(auto& b:batches) {
        b.mesh->generateTangents();auto& m=scene.spawn("Desert detail cell").addComponent<MeshComponent>("Baked desert dressing",b.mesh);
        m.color=colors[b.material];MaterialInstance material;material.roughness=b.material==7?.22f:.92f;material.metallic=b.material==7?.65f:0.f;
        if(b.material>=9){material.material=stoneMaterial;m.color=b.material==10?Vec3{.8f,.82f,.85f}:b.material==11?Vec3{1.1f,1.06f,1.f}:Vec3{1,1,1};}
        m.setMaterial(0,material);
    }
}
void Kit::install(Scene& scene,unsigned model,std::span<const std::byte> bytes) {
    if(model>=loaded.size()||loaded[model])throw std::logic_error("Invalid or installed desert model");
    const auto& expected=models[model];
    // Receiver: four 2048 RGBA maps (64 MiB decoded). Cliff models use three.
    GlbImportOptions options;options.optimizeMeshes=false;options.maxTextureBytes=64*1024*1024;
    auto source=GlbImporter::loadMemory(bytes,options);const auto& b=source->localBounds();
    if(!source->warnings.empty()||source->indices.size()!=expected.triangles*3||
        source->materials.size()!=expected.materials||!b.valid||
        magnitude(b.maximum-b.minimum-expected.dimensions)>.03f||std::abs(b.minimum.y)>.01f)
        throw std::runtime_error(std::string("Desert asset contract mismatch: ")+expected.name);
    auto mesh=art::prepareStatic(*source);
    for(const auto& p:instances)if(p.model==model) {
        auto& actor=scene.spawn(expected.name);actor.transform.position=p.position;actor.transform.rotation.y=p.yaw;
        auto& visual=actor.addComponent<MeshComponent>("Desert solar model",mesh);
        visual.color={1,1,1}; // Preserve authored albedo instead of the engine's blue primitive tint.
        visual.transform.scale={p.scale,p.scale,p.scale};visual.allowInstanceReordering=true;
        if(model>=3&&model<=7){MaterialInstance stone;stone.normalScale=1.6f;stone.roughness=.93f;stone.metallic=0;visual.setMaterial(0,stone);}
        if(model==12){RigidBodySettings settings;settings.motion=BodyMotion::Static;
            actor.addComponent<RigidBodyComponent>("Operations house body",settings);
            auto& collider=actor.addComponent<ColliderComponent>("Operations footprint",CollisionShape::box(expected.dimensions*p.scale*.5f));
            collider.transform.position.y=expected.dimensions.y*p.scale*.5f;
        }
        if(model==7) {
            std::vector<Vec3> vertices;vertices.reserve(source->vertices.size());
            for(const auto& v:source->vertices)vertices.push_back(v.position*p.scale);
            RigidBodySettings settings;settings.motion=BodyMotion::Static;
            actor.addComponent<RigidBodyComponent>("Arch rock body",settings);
            actor.addComponent<ColliderComponent>("Open arch collision",CollisionShape::triangleMesh(vertices,source->indices));
        }
    }
    loaded[model]=true;scene.synchronize();
}
}
