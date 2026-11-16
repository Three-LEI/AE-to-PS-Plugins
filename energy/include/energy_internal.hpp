#pragma once
#include "energy_core.hpp"
namespace energy::detail {
bool rasterize(const Image&,const Parameters&,const std::vector<Segment>&,std::vector<float>&,const Cancel&);
bool composite(const Image&,const Parameters&,const std::vector<float>&,Image&,const Cancel&,const std::vector<float>* flame=nullptr,const std::vector<float>* heat=nullptr);
bool layered_composite(const Image&,const Parameters&,const std::vector<float>&,Image&,const Cancel&,const std::vector<float>*,const std::vector<float>*,const std::vector<float>* seed_override=nullptr);
bool textured_electric(const Image&,const Parameters&,const Path&,Image&,const Cancel&);
bool electric_halo(const Image&,const Parameters&,const std::vector<float>&,std::vector<float>&,const Cancel&);
bool burning_fields(const Image&,const Parameters&,const std::vector<float>&,std::vector<float>&,std::vector<float>&,const Cancel&);
bool unified_render(const Image&,const Parameters&,const Path&,Image&,const Cancel&);
bool extended(const Image&,const Parameters&,const Path&,Image&,const Cancel&);
}
