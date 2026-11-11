// Calls the compiled .8bf through its public entry point with a mock host.
// This is not a substitute for loading the filter in Photoshop.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "PIFilter.h"
#include "fractal_core.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "../../native/tests/legacy_pipl.hpp"

namespace {
void check(bool condition,const char* message) {
    if(!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct MockHandle { char* bytes; int32 size; };
Handle new_handle(int32 n) {
    auto* h=new MockHandle{new char[static_cast<std::size_t>(n)]{},n};
    return reinterpret_cast<Handle>(h);
}
void dispose(Handle h) {
    auto* p=reinterpret_cast<MockHandle*>(h); delete[] p->bytes; delete p;
}
int32 handle_size(Handle h) { return reinterpret_cast<MockHandle*>(h)->size; }
Ptr lock(Handle h,Boolean) { return reinterpret_cast<MockHandle*>(h)->bytes; }
void unlock(Handle) {}
Boolean cancel_requested=false;
Boolean test_abort() { return cancel_requested; }
int progress_calls=0;
void progress(int32 done,int32 total) {
    check(done>=0 && done<=total,"progress range"); ++progress_calls;
}
using Entry=void (*)(int16,FilterRecord*,intptr_t*,int16*);
struct Host {
    HandleProcs handles{};
    BigDocumentStruct big{};
    FilterRecord f{};
    intptr_t data=0;
    Entry entry;
    explicit Host(Entry e,bool big_coordinates=true):entry(e) {
        handles.handleProcsVersion=kCurrentHandleProcsVersion;
        handles.numHandleProcs=kCurrentHandleProcsCount;
        handles.newProc=new_handle; handles.disposeProc=dispose;
        handles.getSizeProc=handle_size; handles.lockProc=lock; handles.unlockProc=unlock;
        f.handleProcs=&handles; f.depth=8; f.imageMode=plugInModeRGBColor; f.planes=4;
        f.abortProc=test_abort; f.progressProc=progress;
        f.bigDocumentData=big_coordinates ? &big : nullptr;
    }
    ~Host() { if(f.parameters) dispose(f.parameters); }
    int16 call(int16 selector) {
        int16 result=32767; entry(selector,&f,&data,&result); return result;
    }
    void set_rect(VRect r) {
        big.filterRect32=r;
        f.filterRect={static_cast<int16>(r.top),static_cast<int16>(r.left),static_cast<int16>(r.bottom),static_cast<int16>(r.right)};
    }
    VRect requested() const {
        if(f.bigDocumentData) return big.inRect32;
        return {f.inRect.top,f.inRect.left,f.inRect.bottom,f.inRect.right};
    }
};
bool same_rect(VRect a,VRect b) { return a.top==b.top && a.left==b.left && a.bottom==b.bottom && a.right==b.right; }
void render_test(Entry entry,bool modern) {
    Host host(entry,modern);
    constexpr int width=1036,height=142,stride=width*3+11;
    std::vector<unsigned char> source(stride*height),target,expected;
    for(std::size_t i=0;i<source.size();++i) source[i]=static_cast<unsigned char>((i*73+i/17)%256);
    target=source; expected=source;
    const VRect area{5,7,136,1032}; // 1025 x 131 => 9 irregular tiles.
    host.set_rect(area);
    const auto offset=area.top*stride+area.left*3;
    check(fractal::process(source.data()+offset,source.size()-offset,stride,expected.data()+offset,expected.size()-offset,stride,1025,131,3,{}, {7,5,1}),"reference fractal core");
    check(host.call(filterSelectorPrepare)==noErr,"prepare");
    check(host.f.bufferSpace==0 && host.f.bufferSpace64==0,"no host scratch request");
    check(host.call(filterSelectorStart)==noErr,"start without UI");
    check(host.f.parameters && host.f.autoMask,"persistent parameters and host selection mask");
    int count=0;
    while(true) {
        const auto r=host.requested();
        if(r.bottom==r.top || r.right==r.left) break;
        check(++count<=9,"tile loop terminates");
        check(r.top>=area.top && r.bottom<=area.bottom && r.left>=area.left && r.right<=area.right,"tile bounds");
        check(r.bottom-r.top<=64 && r.right-r.left<=512,"tile dimensions");
        check(host.f.inLoPlane==0 && host.f.inHiPlane==2 && host.f.outLoPlane==0 && host.f.outHiPlane==2,"RGB planes only");
        const VRect written=modern ? host.big.outRect32 : VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};
        check(same_rect(r,written),"input and output requests agree");
        const auto start=r.top*stride+r.left*3;
        host.f.inData=source.data()+start; host.f.outData=target.data()+start;
        host.f.inRowBytes=host.f.outRowBytes=stride;
        host.f.inColumnBytes=host.f.outColumnBytes=3;
        host.f.inPlaneBytes=host.f.outPlaneBytes=1;
        check(host.call(filterSelectorContinue)==noErr,"continue");
    }
    check(count==9,"all irregular tiles visited");
    check(target==expected,"output matches core; outside rectangle and padding unchanged");
    check(host.call(filterSelectorFinish)==noErr && host.data==0,"finish");
    check(host.call(filterSelectorStart)==noErr,"repeat with saved parameters");
    cancel_requested=true;
    check(host.call(filterSelectorContinue)==userCanceledErr,"cancel propagates");
    check(same_rect(host.requested(),{}),"cancel clears requests");
    cancel_requested=false;
    check(host.call(filterSelectorFinish)==noErr,"finish after cancel");
}
} // namespace

int main() {
    wchar_t path[32768];
    const auto n=GetModuleFileNameW(nullptr,path,32768);
    check(n>0 && n<32768,"test executable path");
    std::wstring plugin(path,n);
    plugin.resize(plugin.find_last_of(L"\\/")+1); plugin+=L"ZaYuFractalNoise.8bf";
    const auto module=LoadLibraryW(plugin.c_str());
    check(module!=nullptr,"Windows loads .8bf and dependencies");
    auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"PluginMain"));
    check(entry!=nullptr,"PluginMain export");
    check(FindResourceW(module,L"IMGUI_LICENSE",RT_RCDATA)!=nullptr,"embedded UI library license");
    const auto properties=legacy_pipl(module);
    check(properties.at("cmpt").find("ZaYuFractalNoiseReconstruction")!=std::string::npos,"embedded identity");
    check(properties.at("catg").find("ZaYu-AE Recreated Effects")!=std::string::npos,"branded menu group embedded");
    const auto cases=properties.at("fici");
    check(cases.size()==28,"seven binary filter cases");
    for(int i=0;i<7;++i){
        if(i==3||i==4)check(cases[i*4]==filterDataHandlingWhiteZap&&cases[i*4+1]==filterDataHandlingFillMask&&cases[i*4+2]==(1<<PIFilterWorksWithBlankDataBit),"editable cases generate blank alpha while preserving host selection");
        else check(cases[i*4+1]!=filterDataHandlingFillMask,"protected/flat cases do not request alpha filling");
    }
    const auto dialog=FindResourceW(module,MAKEINTRESOURCEW(101),RT_DIALOG);
    check(dialog!=nullptr,"parameter dialog resource");
    const auto dialog_memory=LoadResource(module,dialog);
    const std::string dialog_bytes(static_cast<const char*>(LockResource(dialog_memory)),SizeofResource(module,dialog));
    for(const auto* text:{L"Fractal Noise 分形杂色",L"预览 / Preview",L"确定 / OK",L"取消 / Cancel"}) {
        const std::wstring wide(text);
        check(dialog_bytes.find(std::string(reinterpret_cast<const char*>(wide.data()),wide.size()*sizeof(wchar_t)))!=std::string::npos,"exact UTF-16 bilingual dialog text; no mojibake");
    }
    render_test(entry,true);
    render_test(entry,false);
    {
        Host host(entry); host.set_rect({0,0,1,1});
        host.f.depth=12;
        check(host.call(filterSelectorStart)==filterBadMode,"unsupported depth rejected");
        host.f.depth=8; host.f.imageMode=plugInModeCMYKColor;
        check(host.call(filterSelectorStart)==filterBadMode,"unsupported mode rejected");
        host.f.imageMode=plugInModeRGBColor;
        check(host.call(filterSelectorStart)==noErr,"start valid mode");
        unsigned char input[3]{},output[3]{};
        host.f.inData=input; host.f.outData=output;
        host.f.inRowBytes=host.f.outRowBytes=3;
        host.f.inColumnBytes=4;
        check(host.call(filterSelectorContinue)==filterBadParameters,"unsupported layout rejected");
    }
    check(progress_calls==18,"progress callback count");
    for(int depth:{16,32}){
        Host host(entry);host.set_rect({0,0,3,5});host.f.depth=depth;host.f.imageMode=depth==16?plugInModeRGB48:plugInModeRGB96;
        const int bytes=depth==16?2:4,row=5*3*bytes+7;std::vector<unsigned char> src(row*3,0),dst(row*3,211),ref=dst;
        check(fractal::process(src.data(),src.size(),row,ref.data(),ref.size(),row,5,3,3,{}, {},depth==16?fractal::SampleType::photoshop_u16:fractal::SampleType::f32),"deep reference");
        check(host.call(filterSelectorStart)==noErr,"start deep RGB mode");host.f.inData=src.data();host.f.outData=dst.data();host.f.inRowBytes=host.f.outRowBytes=row;host.f.inColumnBytes=host.f.outColumnBytes=3*bytes;host.f.inPlaneBytes=host.f.outPlaneBytes=bytes;
        check(host.call(filterSelectorContinue)==noErr&&dst==ref,"deep output and padding match");check(host.call(filterSelectorFinish)==noErr,"deep finish");
    }
    {
        struct OldP{double contrast=0,brightness=0,scale=100,rotation=0,offset_x=12,offset_y=34,complexity=6,sub_influence=70,sub_scale=56,evolution=90,opacity=45;std::uint32_t seed=123;bool invert=true;};
        struct OldSaved{unsigned long magic=0x46524e31;OldP p;};
        Host host(entry);host.set_rect({0,0,1,1});OldSaved old;host.f.parameters=new_handle(sizeof(old));std::memcpy(lock(host.f.parameters,true),&old,sizeof(old));
        check(host.call(filterSelectorStart)==noErr,"legacy parameter migration");
        struct NewSaved{unsigned long magic;fractal::Parameters p;};check(handle_size(host.f.parameters)==sizeof(NewSaved),"migrated handle resized");NewSaved saved;std::memcpy(&saved,lock(host.f.parameters,true),sizeof(saved));
        check(saved.magic==0x46524e32&&saved.p.seed==123&&saved.p.opacity==45&&saved.p.offset_x==12&&saved.p.invert&&saved.p.fractal==fractal::FractalType::basic&&saved.p.overflow==fractal::Overflow::clip,"legacy values retained with old overflow behavior");
    }
    {
        Host host(entry);host.big.imageSize32={300,500};host.set_rect({70,100,72,104});
        struct Saved{unsigned long magic=0x46524e32;fractal::Parameters p;};Saved saved;
        saved.p.offset_x=17;saved.p.offset_y=23;saved.p.rotation=58;saved.p.scale=234;
        host.f.parameters=new_handle(sizeof(saved));std::memcpy(lock(host.f.parameters,true),&saved,sizeof(saved));
        std::vector<unsigned char> src(24,255),dst(24),expected(24);
        check(fractal::process(src.data(),src.size(),12,expected.data(),expected.size(),12,4,2,3,saved.p,{100,70,1,250,150}),"fixed-center reference for cropped host region");
        check(host.call(filterSelectorStart)==noErr,"start cropped document with saved transform");
        host.f.inData=src.data();host.f.outData=dst.data();host.f.inRowBytes=host.f.outRowBytes=12;host.f.inColumnBytes=host.f.outColumnBytes=3;host.f.inPlaneBytes=host.f.outPlaneBytes=1;
        check(host.call(filterSelectorContinue)==noErr&&dst==expected,"host transform center uses full document dimensions");
        check(host.call(filterSelectorFinish)==noErr,"centered crop finish");
    }
    FreeLibrary(module);
    std::cout << "PASS: Windows DLL load; PluginMain; exact Unicode bilingual dialog/PiPL resources; 32/16-bit coordinate paths; 9 tiles each; saved parameters; selection rectangle; padding; cancel; invalid mode/layout.\n";
    std::cout << "NOTE: Mock-host test only. Photoshop loading, real alpha/selection behavior, dialog interaction and AE parity remain unverified.\n";
}

