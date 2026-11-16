#pragma once
#include "../../native/common/preview_cancel.hpp"
// View-only: render the opacity pattern, then composite with the original
// source's RGB/Alpha. Never soften the photograph/text just to preview a wipe.
namespace blinds {
inline psui::SpatialFrame<Image> preview_mask(const Image& source,const Parameters& p,psui::PreviewPass pass,const Cancel& cancel){
 psui::SpatialFrame<Image> result;result.space=psui::preview_space(source,pass);const auto& s=result.space;
 Image field{int(std::ceil(source.width/s.step)),int(std::ceil(source.height/s.step)),0,0,{}};
 field.rgba.resize(std::size_t(field.width)*field.height*4);
 for(std::size_t i=3;i<field.rgba.size();i+=4)field.rgba[i]=1;
 auto q=p;q.width=std::max(.0001,p.width/s.step);
 // Downsampled cells represent their native area centres. Also scale the
 // native feather sigma, including its constant term, before converting back.
 const double center_shift=.5625*(1-1/s.step);
 q.cx=s.x(p.cx)-center_shift;q.cy=s.y(p.cy)-center_shift;
 q.feather=p.feather>0?std::max(0.,((.38575*p.feather+.3735)/s.step-.3735)/.38575):0;
 const Cancel execute=pass.draft?Cancel(pscpu::SerialPreviewCancel{cancel}):cancel;
 render(field,q,result.image,execute);return result;
}
}
