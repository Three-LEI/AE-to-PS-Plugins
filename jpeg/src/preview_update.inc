void update(psui::LibraryPanel& ui,Context& c){
 try{
  const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass draft,const std::function<bool()>& cancel){
   psui::SpatialFrame<Image> frame;const auto space=psui::preview_space(c.source,draft);frame.space=space;
   const auto& source=c.adaptive_cache.get(c.source,space,cancel);const auto& mask=c.adaptive_cache.get(c.mask,space,cancel);
   if(cancel())return frame;const auto q=space.step>1?draft_parameters(p,space):p;
   render(source,q,frame.image,mask.valid()?&mask:nullptr,cancel);
   return frame;
  })){
   if(frame.image.valid()){c.last=std::move(frame.image);c.adaptive_mapping=frame.space; c.display_dirty=true;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.adaptive_selection,c.adaptive_selection_geometry,c.display,c.source,c.last,c.adaptive_mapping,c.preview,c.display_dirty,c.error);
}
