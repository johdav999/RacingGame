#pragma once
#include "engine/texture.hpp"
#include <algorithm>
#include <cmath>
namespace racing {
// A neutral-panel mask: colour accents and dark glass/trim retain source texels.
// Prepared once per chassis. Painted albedo is capped at 1024; source assets stay immutable.
struct PaintSurface {
    std::shared_ptr<engine::Texture> base;
    std::vector<unsigned char> mask;
    void prepare(const engine::Texture& source) {
        base=std::make_shared<engine::Texture>();
        base->width=std::min(1024u,source.width);base->height=std::min(1024u,source.height);
        base->wrapU=source.wrapU;base->wrapV=source.wrapV;
        base->pixels.resize(base->width*base->height*4);mask.resize(base->width*base->height);
        for(unsigned y=0;y<base->height;++y)for(unsigned x=0;x<base->width;++x) {
            auto i=y*base->width+x,src=((y*source.height/base->height)*source.width+x*source.width/base->width)*4;
            std::copy_n(source.pixels.data()+src,4,base->pixels.data()+i*4);
            float r=source.pixels[src]/255.f,g=source.pixels[src+1]/255.f,b=source.pixels[src+2]/255.f;
            float high=std::max({r,g,b}),low=std::min({r,g,b});
            float neutral=1-std::clamp((high-low-.035f)/.12f,0.f,1.f);
            float panel=std::clamp((low-.28f)/.30f,0.f,1.f);
            panel=panel*panel*(3-2*panel);
            mask[i]=static_cast<unsigned char>(255*panel*neutral);
        }
    }
    std::shared_ptr<const engine::Texture> recolor(int rgb) const {
        auto out=std::make_shared<engine::Texture>(*base);out->name="Custom body paint";
        float color[]={float((rgb>>16)&255)/255,float((rgb>>8)&255)/255,float(rgb&255)/255};
        for(std::size_t i=0;i<mask.size();++i)if(mask[i]) {
            float weight=mask[i]/255.f;
            float shade=std::max({base->pixels[i*4],base->pixels[i*4+1],base->pixels[i*4+2]})/255.f;
            for(int c=0;c<3;++c)out->pixels[i*4+c]=static_cast<unsigned char>(std::clamp(base->pixels[i*4+c]*(1-weight)+255*color[c]*shade*weight,0.f,255.f));
        }
        return out;
    }
};
}
