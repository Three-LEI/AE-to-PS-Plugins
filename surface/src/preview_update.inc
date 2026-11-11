// Shared preview flow adapted from the current Lens production panel.
void update(psui::LibraryPanel& ui,Context& c){
 try{Image result;if(c.worker.take(result)){if(result.valid()){c.last=std::move(result);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.dirty&&GetTickCount64()>=c.due&&!c.error){const auto p=c.p;c.worker.start([&c,p](std::stop_token token){Image result;render(c.source,p,result,c.map.valid()?&c.map:nullptr,[token]{return token.stop_requested();});return result;});c.dirty=false;}
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int step=std::max(1,int(std::ceil(double(std::max(roi.right-roi.left,roi.bottom-roi.top))/c.view.proxy_side())));
 const int w=std::max(1,(roi.right-roi.left+step-1)/step),h=std::max(1,(roi.bottom-roi.top+step-1)/step);
 c.proxy.width=w;c.proxy.height=h;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 if(c.host->haveMask&&(!c.selection.size()||c.selection_geometry.width!=w||c.selection_geometry.height!=h||c.selection_geometry.origin_x!=roi.left||c.selection_geometry.origin_y!=roi.top||c.selection_geometry.step!=step)){
  auto* channel=c.host->documentInfo?c.host->documentInfo->selection:nullptr;
  if(channel&&!psui::host_read_preview_channel(c.host,channel,c.proxy,c.selection))c.selection_geometry=c.proxy;else c.selection.clear();
 }
 c.background.prepare(c.host,c.proxy);c.display.resize(std::size_t(w)*h*3);const auto& result=c.preview&&c.last.valid()?c.last:c.source;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){const int xx=std::clamp(roi.left+x*step-c.source.left,0,c.source.width-1),yy=std::clamp(roi.top+y*step-c.source.top,0,c.source.height-1);const auto n=(std::size_t(yy)*c.source.width+xx)*4;
  std::array<float,4> before{},after{};for(int k=0;k<4;++k){before[k]=c.source.rgba[n+k];after[k]=result.rgba[n+k];}
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto colour=c.background.pixel(x,y,before,after,c.selection.empty()?1:c.selection[std::size_t(y)*w+x]);for(int k=0;k<3;++k)c.display[(std::size_t(y)*w+x)*3+k]=colour[k];
 }
 if(!ui.preview(c.host,c.display.data(),w,h))c.error=memFullErr;c.display_dirty=c.view.dirty=false;
}
