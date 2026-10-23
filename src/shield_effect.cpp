#include "shield_effect.hpp"
#include <cmath>

namespace racing {
using namespace engine;
namespace {
constexpr float impactDuration=.48f;
std::shared_ptr<const Texture> lattice() {
    static const auto shared=[] {
        auto t=std::make_shared<Texture>();t->name="Shield / antialiased hex lattice";
        t->width=512;t->height=512;t->pixels.resize(512*512*4);
        constexpr float root3=1.7320508f;
        for(unsigned y=0;y<t->height;++y)for(unsigned x=0;x<t->width;++x) {
            float px=(x+.5f)/512.f*root3*8,py=(y+.5f)/512.f*12;
            int row=static_cast<int>(std::round(py/1.5f));float nearest=100;
            for(int r=row-1;r<=row+1;++r) {
                float offset=(r%2!=0)?.5f:0.f;
                int col=static_cast<int>(std::round(px/root3-offset));
                float dx=std::abs(px-root3*(col+offset)),dy=std::abs(py-1.5f*r);
                nearest=std::min(nearest,std::max(dx,.5f*dx+.8660254f*dy));
            }
            float edge=1-std::clamp(std::abs(nearest-.8660254f)/.055f,0.f,1.f);
            // Fade polar crowding instead of turning the poles into bright knots.
            float pole=std::clamp(std::min(y+.5f,511.5f-y)/48.f,0.f,1.f);
            auto i=(y*512+x)*4;
            t->pixels[i]=t->pixels[i+1]=t->pixels[i+2]=255;
            t->pixels[i+3]=static_cast<std::uint8_t>(255*(.018f+.62f*edge*edge*pole));
        }
        return t;
    }();return shared;
}
std::shared_ptr<const Mesh> ring() {
    static const auto shared=[] {
        auto mesh=std::make_shared<Mesh>();
        // A feathered annulus: transparent edges, luminous middle. Shared by
        // the silhouette, travelling scan and every curved-surface impact ring.
        auto texture=std::make_shared<Texture>();texture->width=1;texture->height=64;
        texture->wrapV=TextureWrap::Clamp;texture->pixels.resize(64*4);
        for(int y=0;y<64;++y) {
            float a=std::sin(pi*(y+.5f)/64);a=a*a;
            for(int c=0;c<3;++c)texture->pixels[y*4+c]=255;
            texture->pixels[y*4+3]=static_cast<std::uint8_t>(a*255);
        }
        auto mat=std::make_shared<Material>();mat->shader=MaterialShader::Unlit;
        mat->alphaMode=AlphaMode::Additive;mat->doubleSided=true;mat->baseColorTexture=texture;
        mesh->materials.push_back(mat);
        for(unsigned i=0;i<=96;++i) {
            float a=2*pi*i/96;
            for(int side=0;side<2;++side) {
                float r=side?1.f:.955f;
                mesh->vertices.push_back({{std::cos(a)*r,std::sin(a)*r,0},{0,0,1},{float(i)/96,float(side)}});
            }
            if(i<96){auto n=i*2;mesh->indices.insert(mesh->indices.end(),{n,n+1,n+2,n+1,n+3,n+2});}
        }
        mesh->generateTangents();return mesh;
    }();return shared;
}
Vec3 facing(Vec3 n) {return {-std::asin(std::clamp(n.y,-1.f,1.f)),std::atan2(n.x,n.z),0};}
void configure(MeshComponent& mesh) {
    mesh.color={.06f,.7f,1.f};mesh.blend=BlendMode::Additive;
    mesh.castsShadow=false;mesh.receivesShadow=false;mesh.receivesDecals=false;
}
}
ShieldEffect::ShieldEffect():Component("Vehicle shield field") {
    static const auto sphere=Mesh::sphere(64,32);
    shell=&addChild<MeshComponent>("Shield membrane",sphere);configure(*shell);
    shell->transform.scale={2,2,2}; // Engine primitive has radius one half.
    MaterialInstance material;material.shader=MaterialShader::Unlit;
    material.alphaMode=AlphaMode::Additive;material.doubleSided=false;
    material.baseColorTexture=lattice();shell->setMaterial(0,material);
    rim=&addChild<MeshComponent>("Shield silhouette",ring());configure(*rim);
    scan=&addChild<MeshComponent>("Shield scan",ring());configure(*scan);
    for(auto& r:ripples){r.mesh=&addChild<MeshComponent>("Shield impact",ring());configure(*r.mesh);}
    fit({},radii);reset();
}
void ShieldEffect::fit(Vec3 c,Vec3 r) {
    if(!std::isfinite(c.x+c.y+c.z+r.x+r.y+r.z)||r.x<=0||r.y<=0||r.z<=0)
        throw std::invalid_argument("Invalid shield envelope");
    center=c;radii=r;transform.position=center;transform.scale=radii;
}
void ShieldEffect::reset() {
    envelope=clock=collapse=ignition=0;energy=1;active=false;hitPending=false;nextRipple=0;enabled=false;
    for(auto& r:ripples){r.remaining=0;r.mesh->enabled=false;}
}
void ShieldEffect::impact(Vec3 direction) {
    // Convert the attack direction from vehicle space to unit ellipsoid space.
    Vec3 n{direction.x/radii.x,direction.y/radii.y,direction.z/radii.z};
    if(!std::isfinite(n.x+n.y+n.z)||dot(n,n)<1e-6f)n={0,0,-1};
    auto& r=ripples[nextRipple++%ripples.size()];r.direction=normalized(n);r.remaining=impactDuration;
    hitPending=true;
}
float ShieldEffect::impactRay(Vec3 origin,Vec3 direction) {
    const auto p=origin-center;
    Vec3 o{p.x/radii.x,p.y/radii.y,p.z/radii.z};
    Vec3 d{direction.x/radii.x,direction.y/radii.y,direction.z/radii.z};
    float a=dot(d,d),b=dot(o,d),c=dot(o,o)-1,discriminant=b*b-a*c;
    if(a>1e-8f&&discriminant>=0) {
        float entry=(-b-std::sqrt(discriminant))/a;
        if(entry>=0) {impact(p+direction*entry);return entry;}
    }
    // A shooter already inside the field still gets bounded block feedback.
    impact(direction*-1.f);return -1;
}
unsigned ShieldEffect::activeImpacts() const {
    unsigned count=0;for(const auto& r:ripples)if(r.remaining>0)++count;return count;
}
void ShieldEffect::advance(float dt,const CombatState& combat) {
    if(!std::isfinite(dt)||dt<=0)return;
    clock=std::fmod(clock+dt,120.f);
    ignition=std::max(0.f,ignition-dt);collapse=std::max(0.f,collapse-dt);
    if(combat.shield&&!active)ignition=.24f;
    if((active||hitPending)&&!combat.shield&&combat.releaseRequired)collapse=.36f;
    hitPending=false;
    active=combat.shield;
    energy=std::clamp(combat.charge/std::max(1.f,combat.capacity),0.f,1.f);
    envelope=std::clamp(envelope+dt*(active?8.f:-5.f),0.f,1.f);
    for(auto& r:ripples)r.remaining=std::max(0.f,r.remaining-dt);
}
void ShieldEffect::present(Vec3 worldEye) {
    enabled=envelope>0||collapse>0||activeImpacts()>0;
    if(!enabled)return;
    float breaking=collapse/.36f;
    float expansion=1+(collapse>0?.1f*(1-breaking):.018f*std::sin(clock*2.2f));
    transform.position=center;transform.scale=radii*expansion;
    const Vec3 blue{.07f,.62f,1.f},low{1.f,.35f,.07f};
    float warning=active?std::clamp((.25f-energy)*4,0.f,1.f):0;
    Vec3 color=blue*(1-warning)+low*warning;
    if(collapse>0)color=low;
    float opacity=std::max(envelope,breaking*.65f);
    shell->color=color;shell->opacity=opacity*(.48f+.08f*std::sin(clock*2.2f)+ignition);
    shell->materialOverrides[0]->uvOffset={clock*.008f,0};
    // Analytic silhouette in ellipsoid space. Perspective tangency keeps the
    // rim on the shell, including oblique chase-camera views and vehicle roll.
    auto eye=transformPoint(inverseAffine(worldMatrix()),worldEye);
    float distance=std::sqrt(dot(eye,eye));
    rim->enabled=distance>1.02f;
    if(rim->enabled) {
        auto n=eye*(1/distance);float radius=std::sqrt(1-1/(distance*distance));
        rim->transform.position=n*(1/distance);rim->transform.rotation=facing(n);
        rim->transform.scale={radius,radius,radius};rim->color=color;rim->opacity=opacity*.7f;
    }
    float height=-.8f+1.6f*std::fmod(clock*.38f,1.f);
    float radius=std::sqrt(1-height*height);
    scan->transform.position={0,height,0};scan->transform.rotation={pi*.5f,0,0};
    scan->transform.scale={radius,radius,radius};scan->color=color;
    scan->opacity=opacity*.22f;scan->enabled=envelope>0;
    for(auto& r:ripples) {
        r.mesh->enabled=r.remaining>0;if(!r.mesh->enabled)continue;
        float age=1-r.remaining/impactDuration,angle=.08f+age*1.05f;
        float radius=std::sin(angle)*1.012f;
        r.mesh->transform.position=r.direction*(std::cos(angle)*1.012f);
        r.mesh->transform.rotation=facing(r.direction);r.mesh->transform.scale={radius,radius,radius};
        r.mesh->color={.55f,.9f,1.f};r.mesh->opacity=(1-age)*(1-age);
    }
}
}
