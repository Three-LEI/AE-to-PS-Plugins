#pragma once
#include "flare_design.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
namespace flare {
// Kernel work runs off the UI thread. Texture creation remains on the UI
// thread, and closing the dialog cancels and joins its own worker.
class ThumbnailRenderer {
public:
 static constexpr int width=640,height=400;
 struct Frame {std::vector<std::uint8_t> rgba;};
 void start(){if(worker.joinable()||failed)return;try{worker=std::thread([this]{try{
  Image source{width,height,0,0,std::vector<float>(width*height*4,0)};for(std::size_t i=3;i<source.rgba.size();i+=4)source.rgba[i]=1;
  for(int i=0;i<builtin_preset_count&&!stop;++i){auto value=design_preset(i,width,height);value.style.hx=width*.3125;value.style.hy=height*.34375;value.style.px=width*.5859375;value.style.py=height*.59375;Image out;
   if(!render(source,value.style,out,nullptr,[this]{return stop.load();},{},&value.design)){if(!stop)failed=true;break;}
   auto frame=std::make_shared<Frame>();frame->rgba.resize(width*height*4);for(std::size_t n=0;n<frame->rgba.size();++n)frame->rgba[n]=std::uint8_t(std::lround(std::clamp(out.rgba[n],0.f,1.f)*255));
   std::lock_guard guard(mutex);frames[i]=std::move(frame);
  }
 }catch(...){failed=true;}});}catch(...){failed=true;}}
 bool error()const{return failed.load();}
 std::shared_ptr<const Frame> get(int index){std::lock_guard guard(mutex);return index>=0&&index<builtin_preset_count?frames[index]:nullptr;}
 ~ThumbnailRenderer(){stop=true;if(worker.joinable())worker.join();}
 ThumbnailRenderer()=default;ThumbnailRenderer(const ThumbnailRenderer&)=delete;ThumbnailRenderer& operator=(const ThumbnailRenderer&)=delete;
private:
 std::atomic_bool stop=false,failed=false;std::thread worker;std::mutex mutex;std::array<std::shared_ptr<const Frame>,builtin_preset_count> frames;
};
}
