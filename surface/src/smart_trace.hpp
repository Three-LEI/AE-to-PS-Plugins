#pragma once
#ifdef SURFACE_SMART_TRACE
#include <fstream>
#include <filesystem>
namespace surface {
inline void smart_trace_seed(Parameters& p){
 const std::filesystem::path folder=std::filesystem::temp_directory_path()/"AE-to-PS-surface-smart";
 if(std::filesystem::exists(folder/"seed-parameters.txt")){p.softness=7;p.height=41;p.displacement=73;p.ambient=67;p.diffuse=23;p.specular=36;p.roughness=.11;p.metal=17;p.light_type=1;p.light_x=17;p.light_y=29;p.light_color={.8,.4,.2};p.map_key=psui::layer_name_key("QA external map");}
}
inline void smart_trace(int16 selector,FilterRecord* f,intptr_t data,int16 result){
 const std::filesystem::path folder=std::filesystem::temp_directory_path()/"AE-to-PS-surface-smart";
 if(selector==filterSelectorAbout||!f||!std::filesystem::exists(folder/"enable-trace.txt"))return;
 try {
  std::ofstream out(folder/"trace.tsv",std::ios::app);
  out<<(effect_kind==Kind::blob?"blob":"glass")<<'\t'<<selector<<'\t'<<data<<'\t'<<result;
  std::ifstream stage_file(folder/"stage.txt");std::string stage;std::getline(stage_file,stage);out<<"\tstage="<<stage;
  if(f->bigDocumentData){const auto& a=f->bigDocumentData->filterRect32;const auto& b=f->bigDocumentData->outRect32;out<<"\tfilter="<<a.left<<','<<a.top<<','<<a.right<<','<<a.bottom<<"\tout="<<b.left<<','<<b.top<<','<<b.right<<','<<b.bottom;}
  out<<"\tstrides="<<f->outRowBytes<<','<<f->outColumnBytes<<','<<f->outPlaneBytes<<"\tplanes="<<f->outLoPlane<<','<<f->outHiPlane<<"\tcase="<<int(f->filterCase);
  if(f->descriptorParameters)out<<"\tdesc="<<(f->descriptorParameters->descriptor?1:0)<<"\tplay="<<f->descriptorParameters->playInfo<<"\trecord="<<f->descriptorParameters->recordInfo;
  if(f->parameters&&f->handleProcs){auto* h=f->handleProcs;const auto bytes=h->getSizeProc(f->parameters);out<<"\tbytes="<<bytes;
   struct Saved{unsigned long magic;Parameters p;};if(bytes==sizeof(Saved)){Saved saved{};const auto* memory=h->lockProc(f->parameters,true);if(memory){std::memcpy(&saved,memory,sizeof(saved));h->unlockProc(f->parameters);out<<"\tsoft="<<saved.p.softness<<"\theight="<<saved.p.height<<"\tmap="<<saved.p.map_key<<"\tpoint="<<saved.p.light_x<<','<<saved.p.light_y;}}
  }
  if(selector==filterSelectorStart&&f->documentInfo){auto* d=f->documentInfo;out<<"\tbounds="<<d->bounds.left<<','<<d->bounds.top<<','<<d->bounds.right<<','<<d->bounds.bottom;for(const auto& layer:psui::host_layers(f))out<<"\tlayer="<<layer.id<<':'<<layer.name<<':'<<layer.target;}
  out<<'\n';
  if(selector==filterSelectorStart&&result==noErr)std::filesystem::remove(folder/"seed-parameters.txt");
 }catch(...){ }
}
}
#endif
