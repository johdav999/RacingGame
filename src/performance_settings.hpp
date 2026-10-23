#pragma once
#include "engine/render_quality.hpp"

namespace racing {
// Game policy over the pinned engine's portable quality contract.
inline engine::RenderQuality performanceQuality(int preset, int scalePercent) {
    if(preset<0 || preset>2 || scalePercent<50 || scalePercent>100 || scalePercent%5)
        throw std::invalid_argument("Invalid racing performance settings");
    auto q=engine::RenderQuality::profile(static_cast<engine::QualityProfile>(preset));
    q.resolution.scale=scalePercent/100.f;
    q.resolution.maxPixelRatio=preset==2?2.f:1.f;
    q.resolution.maxPixels=preset==0?2073600:preset==1?3686400:8294400;
    q.maxShadowMaps=preset==0?0:preset==1?1:4;
    q.msaaSamples=preset==2?4:1;
    q.validate();
    return q;
}
}
