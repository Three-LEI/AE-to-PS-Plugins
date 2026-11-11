#include "../../displacement/tests/mock_host.hpp"
#include "vector_host.hpp"
#include <fstream>
#include <iostream>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
int main(int argc,char** argv){
 if(argc!=3)return 1;std::ifstream input(argv[1],std::ios::binary);std::string header;int width,height,maximum;
 input>>header>>width>>height>>maximum;input.get();if(header!="P6"||maximum!=255||width<=0||height<=0)return 2;
 mock::Host host(width,height);for(std::size_t i=0;i<host.pixels[0].size();i+=4){for(int k=0;k<3;++k)host.pixels[0][i+k]=std::uint8_t(input.get())/255.f;host.pixels[0][i+3]=1;}
 if(!input)return 3;
 struct Saved{unsigned long magic=0x5A564231;vectorblur::Parameters p;};Saved saved;saved.p.amount=50;saved.p.quality=3;
 host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
 wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);std::wstring path(exe);path.resize(path.find_last_of(L"\\/")+1);path+=L"ZaYuVectorBlur.8bf";
 const auto library=LoadLibraryW(path.c_str());if(!library)return 4;const auto entry=reinterpret_cast<Entry>(GetProcAddress(library,"PluginMain"));if(!entry)return 5;
 intptr_t state=0;auto call=[&](int selector){int16 status;entry(int16(selector),&host.f,&state,&status);return status;};
 if(call(filterSelectorStart)!=noErr)return 6;
 std::vector<std::uint8_t> output(std::size_t(width)*height*4);int tiles=0;
 while(host.big.outRect32.bottom>host.big.outRect32.top){const auto rect=host.big.outRect32;
  host.f.outData=output.data()+(std::size_t(rect.top)*width+rect.left)*4;host.f.outRowBytes=width*4;host.f.outColumnBytes=4;host.f.outPlaneBytes=1;
  if(call(filterSelectorContinue)!=noErr||++tiles>1000)return 7;
 }if(call(filterSelectorFinish)!=noErr||state!=0)return 8;FreeLibrary(library);
 std::ofstream file(argv[2],std::ios::binary);file<<"P6\n"<<width<<' '<<height<<"\n255\n";
 for(std::size_t i=0;i<output.size();i+=4)for(int k=0;k<3;++k)file.put(char(std::lround(output[i+k]*output[i+3]/255.f)));
 std::cout<<"PASS: actual .8bf Natural 50 reference, native RGB8, "<<tiles<<" output tiles, params/alpha/cleanup.\n";
}
