// Shared viewport/background sampling follows the spatial-filter panel contract.
void update(psui::LibraryPanel& ui,Context& c){
 Rendered completed;try{if(c.worker.take(completed)){if(completed.image.valid()){c.last=std::move(completed.image);c.field=std::move(completed.field);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.dirty&&GetTickCount64()>=c.update_at&&!c.error){
  const auto p=c.p;c.worker.start([&c,p](std::stop_token token){Rendered out;render(c.source,p,out.image,c.map.valid()?&c.map:nullptr,[token]{return token.stop_requested();},&out.field);return out;});c.dirty=false;
 }
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int w=roi.right-roi.left,h=roi.bottom-roi.top;
 const int step=std::max(1,int(std::ceil(double(std::max(w,h))/c.view.proxy_side()))),pw=std::max(1,(w+step-1)/step),ph=std::max(1,(h+step-1)/step);
 c.proxy.width=pw;c.proxy.height=ph;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 if(c.host->haveMask){
  auto* channel=c.host->documentInfo?c.host->documentInfo->selection:nullptr;
  if(!channel){c.selection.clear();c.selection_approximate=true;}
  else if(c.selection_geometry.width!=pw||c.selection_geometry.height!=ph||c.selection_geometry.origin_x!=roi.left||c.selection_geometry.origin_y!=roi.top||c.selection_geometry.step!=step){
   const auto error=psui::host_read_preview_channel(c.host,channel,c.proxy,c.selection);
   if(error){c.selection.clear();c.selection_approximate=true;}else{c.selection_geometry=c.proxy;c.selection_approximate=false;}
  }
 }
 c.background.prepare(c.host,c.proxy);c.display.resize(std::size_t(pw)*ph*3);
 const auto& pixels=c.preview&&c.last.valid()?c.last:c.source;
 for(int y=0;y<ph;++y)for(int x=0;x<pw;++x){
  const int xx=std::clamp(roi.left+x*step-pixels.left,0,pixels.width-1),yy=std::clamp(roi.top+y*step-pixels.top,0,pixels.height-1);const auto i=(std::size_t(yy)*pixels.width+xx)*4;
  std::array<float,4> before{},after{};for(int k=0;k<4;++k){before[k]=c.source.rgba[i+k];after[k]=pixels.rgba[i+k];}
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  // RGB-only documents show the same black flattening as the native adapter.
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const float selected=c.selection.empty()?1:std::clamp(c.selection[std::size_t(y)*pw+x],0.f,1.f);
  const auto color=c.background.pixel(x,y,before,after,selected);for(int k=0;k<3;++k)c.display[(std::size_t(y)*pw+x)*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),pw,ph))c.error=memFullErr;
 c.view.dirty=c.display_dirty=false;
}
