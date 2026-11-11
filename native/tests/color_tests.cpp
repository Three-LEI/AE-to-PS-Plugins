#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/color_sampling.hpp"
#include "psui/photoshop_host.hpp"
#include "PIUIHooksSuite.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
int checks=0,calls=0,mode=0;
POINT expected{-320,214};
int acquired=0,released=0,cursor_mode=0;
PSUIHooksSuite1 hooks{};
void require(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void expect_value(double a,double b,const char* message){require(std::abs(a-b)<.00001,message);}
OSErr sample(ColorServicesInfo* info){
 ++calls;require(info->infoSize==sizeof(*info),"SDK record size");
 require(info->selector==plugIncolorServicesSamplePoint,"dedicated host sample selector");
 require(info->sourceSpace==plugIncolorServicesRGBSpace&&info->resultSpace==plugIncolorServicesRGBSpace,"normalized RGB request");
 require(!info->reserved&&!info->reservedSourceSpaceInfo&&!info->reservedResultSpaceInfo,"reserved fields zero");
 auto* point=info->selectorParameter.globalSamplePoint;
 require(point&&point->h==expected.x&&point->v==expected.y,"global screen point; negative monitor coordinates; x/y order");
 info->colorComponents[0]=300;info->colorComponents[1]=128;info->colorComponents[2]=-1;
 if(mode==1)return errInvalidSamplePoint;
 if(mode==2)info->resultSpace=plugIncolorServicesHSBSpace;
 return noErr;
}
SPAPI SPErr cursor(const PICursor_t id){require(std::string(id)==kPICursorEyedropper,"native Photoshop eyedropper cursor ID");if(cursor_mode==2)return -1;::SetCursor(LoadCursorW(nullptr,IDC_HAND));return 0;}
SPAPI SPErr acquire(const char* name,int32 version,const void** suite){++acquired;require(std::string(name)==kPSUIHooksSuite&&version==kPSUIHooksSuiteVersion1,"official UI Hooks suite request");if(cursor_mode==1)return -1;*suite=&hooks;return 0;}
SPAPI SPErr release(const char* name,int32 version){++released;require(std::string(name)==kPSUIHooksSuite&&version==kPSUIHooksSuiteVersion1,"matching UI Hooks release");return 0;}
}
int main(){try{
 psui::PreviewColorSampler sampler;double color[3]{.2,.3,.4};
 require(!sampler.sample(0,0,color),"uninitialized preview rejects sampling");
 const std::uint8_t pixels[]{255,0,0, 0,255,0, 0,0,255, 255,255,255};
 sampler.pixels(pixels,2,2);sampler.geometry({10,20},{110,120},{20,30},{100,110});
 require(sampler.sample(35,45,color),"zoomed pixel center accepted");expect_value(color[0],1,"red texel");expect_value(color[1],0,"red texel green");
 require(sampler.sample(60,70,color),"linear texture interpolation accepted");
 for(double c:color)expect_value(c,.5,"interpolated RGB matches displayed preview");
 for(const auto& point:std::array<ImVec2,7>{{{15,45},{35,25},{100,50},{35,110},{-100,0},{110,60},{std::numeric_limits<float>::infinity(),40}}})
  require(!sampler.sample(point.x,point.y,color),"letterbox, clipped, boundary and nonfinite coordinates rejected");
 sampler.geometry({-90,-80},{110,120},{0,0},{60,70});
 require(sampler.sample(10,20,color),"panned preview maps the same displayed texture");
 for(double c:color)expect_value(c,.5,"pan mapping");
 sampler.visible=false;require(!sampler.sample(10,20,color),"hidden preview cannot be sampled");
 const std::uint8_t single[]{17,34,51};sampler.pixels(single,1,1);sampler.geometry({0,0},{200,100},{0,0},{200,100});
 require(sampler.sample(199,99,color),"single texel edge clamping");expect_value(color[2],.2,"single texel RGB");
 FilterRecord host{};host.colorServices=sample;
 require(psui::host_sample_rgb(&host,expected,color)==noErr,"native canvas sample succeeds");
 expect_value(color[0],1,"host RGB upper bound");expect_value(color[1],128/255.,"host RGB normalization");expect_value(color[2],0,"host RGB lower bound");
 const std::array<double,3> before{color[0],color[1],color[2]};
 for(int error_mode:{1,2}){mode=error_mode;require(psui::host_sample_rgb(&host,expected,color)!=noErr,"host failures propagated");for(int k=0;k<3;++k)expect_value(color[k],before[k],"failure leaves colour unchanged");}
 const int old_calls=calls;
 require(psui::host_sample_rgb(&host,{32768,0},color)==errInvalidSamplePoint,"positive 16-bit overflow rejected");
 require(psui::host_sample_rgb(&host,{-32769,0},color)==errInvalidSamplePoint,"negative 16-bit overflow rejected");
 require(calls==old_calls,"overflow never calls Photoshop with wrapped coordinates");
 require(psui::host_sample_rgb(nullptr,expected,color)==paramErr,"null host rejected");
 host.colorServices=nullptr;require(psui::host_sample_rgb(&host,expected,color)==paramErr,"missing host service rejected");
 require(!psui::screen_sample_rgb({0,0},nullptr),"screen sampler rejects null output");
 require(psui::host_eyedropper_cursor(nullptr)==LoadCursorW(nullptr,IDC_CROSS),"missing Photoshop cursor uses crosshair");
 SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;host.sSPBasic=&basic;hooks.SetCursor=cursor;
 const auto original_cursor=GetCursor();
 require(psui::host_eyedropper_cursor(&host)==LoadCursorW(nullptr,IDC_HAND),"native cursor is borrowed from host");
 require(GetCursor()==original_cursor,"cursor discovery restores previous pointer");
 require(acquired==1&&released==1,"successful UI Hooks acquisition released");
 cursor_mode=1;require(psui::host_eyedropper_cursor(&host)==LoadCursorW(nullptr,IDC_CROSS),"unsupported suite falls back safely");
 require(acquired==2&&released==1,"failed acquisition is not released");
 cursor_mode=2;require(psui::host_eyedropper_cursor(&host)==LoadCursorW(nullptr,IDC_CROSS),"failed native cursor falls back safely");
 require(acquired==3&&released==2,"failed SetCursor still releases suite");
 std::cout<<"PASS: "<<checks<<" colour sampling checks; RGB/interpolation/zoom/pan/clip; SDK selector/global coordinates; range/errors/unchanged failure.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
