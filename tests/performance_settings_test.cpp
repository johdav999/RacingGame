#include "performance_settings.hpp"
#include <iostream>
#include <stdexcept>
void check(bool value) {if(!value)throw std::runtime_error("Performance policy check failed");}
int main() {
    engine::RenderCapabilities caps;
    for(int preset=0;preset<3;++preset) {
        auto full=racing::performanceQuality(preset,100), half=racing::performanceQuality(preset,50);
        auto a=engine::resolveResolution(1280,720,1,full.resolution,caps);
        auto b=engine::resolveResolution(1280,720,1,half.resolution,caps);
        check(a.width==1280 && a.height==720 && b.width==640 && b.height==360);
        auto huge=engine::resolveResolution(7680,4320,3,full.resolution,caps);
        check(std::uint64_t(huge.width)*huge.height<=full.resolution.maxPixels);
        check(full.lod && full.shadowLod);
        check(engine::resolveMsaaSamples(full,caps)==1);
        caps.msaa4x=true;check(engine::resolveMsaaSamples(full,caps)==(preset==2?4u:1u));caps.msaa4x=false;
    }
    check(racing::performanceQuality(0,75).maxShadowMaps==0);
    check(racing::performanceQuality(1,100).maxShadowMaps==1);
    for(auto [preset,scale]:{std::pair{-1,100},{3,100},{1,0},{1,49},{1,101},{1,76}}) {
        bool rejected=false;try {racing::performanceQuality(preset,scale);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected);
    }
    std::cout<<"Performance settings and resolution bounds passed\n";
}
