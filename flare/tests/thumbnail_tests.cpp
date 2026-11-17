#include "../src/thumbnail_renderer.hpp"
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>
int main(){try{
 using namespace flare;const auto begin=std::chrono::steady_clock::now();ThumbnailRenderer renderer;renderer.start();std::set<std::uint64_t> hashes;int ready=0;
 while(ready<builtin_preset_count){if(renderer.error())throw std::runtime_error("background render failed");if(auto image=renderer.get(ready)){
  if(image->rgba.size()!=std::size_t(640*400*4))throw std::runtime_error("native card resolution");std::uint64_t hash=14695981039346656037ull;unsigned colored=0;
  for(std::size_t n=0;n<image->rgba.size();n+=4){if(image->rgba[n]||image->rgba[n+1]||image->rgba[n+2])++colored;if(image->rgba[n+3]!=255)throw std::runtime_error("opaque card alpha");for(int k=0;k<4;k++)hash=(hash^image->rgba[n+k])*1099511628211ull;}
  if(colored<200||!hashes.insert(hash).second||renderer.get(ready)!=image)throw std::runtime_error("nonempty distinct immutable previews");++ready;
 }else std::this_thread::sleep_for(std::chrono::milliseconds(2));if(std::chrono::steady_clock::now()-begin>std::chrono::seconds(20))throw std::runtime_error("background timeout");}
 for(int i=0;i<4;i++){ThumbnailRenderer cancelled;cancelled.start();}
 std::cout<<"PASS: 12 distinct cached native 640x400 previews; RGBA ownership; background completion and early-close cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
