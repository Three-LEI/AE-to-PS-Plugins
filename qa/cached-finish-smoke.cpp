// Compile an existing effect against the optional finish-recording extension.
// No old DLL is rebuilt or replaced.
#include "../displacement/tests/mock_host.hpp"
#define LIGHTFX_KIND 1
#include "../lightfx/src/filter.cpp"
#include <iostream>
namespace lightfx {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&){return IDCANCEL;}}
int main(){for(int depth:{8,16,32}){mock::Host host(96,64,depth);intptr_t state=0;int16 error=0;PluginMain(filterSelectorStart,&host.f,&state,&error);if(error||state<=1)return 2;std::vector<std::uint8_t> output(96*64*4*(depth/8));host.f.outData=output.data();host.f.outRowBytes=96*4*(depth/8);host.f.outColumnBytes=4*(depth/8);host.f.outPlaneBytes=depth/8;PluginMain(filterSelectorContinue,&host.f,&state,&error);if(error)return 3;PluginMain(filterSelectorFinish,&host.f,&state,&error);if(error||state)return 4;}std::cout<<"PASS optional finish hook leaves existing Shine lifecycle unchanged in RGB8/16/32\n";}
