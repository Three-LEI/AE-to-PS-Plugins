// Calls the compiled .8bf through its public entry point with a mock host.
// This is not a substitute for loading the filter in Photoshop.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "PIFilter.h"
#include "tint_core.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "legacy_pipl.hpp"

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
    tint::ConstImage in{source.data()+offset,source.size()-offset,1025,131,stride,3,tint::SampleType::u8};
    tint::Image out{expected.data()+offset,expected.size()-offset,1025,131,stride,3,tint::SampleType::u8};
    check(tint::process(in,out,{})==tint::Status::ok,"reference core");
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
    plugin.resize(plugin.find_last_of(L"\\/")+1); plugin+=L"ZaYuTint.8bf";
    const auto module=LoadLibraryW(plugin.c_str());
    check(module!=nullptr,"Windows loads .8bf and dependencies");
    auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"PluginMain"));
    check(entry!=nullptr,"PluginMain export");
    check(FindResourceW(module,L"IMGUI_LICENSE",RT_RCDATA)!=nullptr,"embedded UI library license");
    const auto properties=legacy_pipl(module);
    check(properties.at("cmpt").find("ZaYuTintReconstruction")!=std::string::npos,"embedded identity");
    check(properties.at("catg").find("ZaYu-AE Recreated Effects")!=std::string::npos,"branded menu group embedded");
    const auto cases=properties.at("fici");
    check(cases.size()==28,"seven binary filter cases");
    for(int i=0;i<7;++i)check(cases[i*4]==(i==2?0:1)&&cases[i*4+1]==(i==2?0:1)&&cases[i*4+2]==0,"Tint preserves input alpha and selection clipping");
    const auto dialog=FindResourceW(module,MAKEINTRESOURCEW(101),RT_DIALOG);
    check(dialog!=nullptr,"parameter dialog resource");
    const auto dialog_memory=LoadResource(module,dialog);
    const std::string dialog_bytes(static_cast<const char*>(LockResource(dialog_memory)),SizeofResource(module,dialog));
    for(const auto* text:{L"色调 / Tint",L"黑色 / Black",L"白色 / White",L"交换 / Swap",L"预览 / Preview",L"确定 / OK",L"取消 / Cancel"}) {
        const std::wstring wide(text);
        check(dialog_bytes.find(std::string(reinterpret_cast<const char*>(wide.data()),wide.size()*sizeof(wchar_t)))!=std::string::npos,"exact UTF-16 bilingual dialog text; no mojibake");
    }
    render_test(entry,true);
    render_test(entry,false);
    {
        Host host(entry); host.set_rect({0,0,1,1});
        host.f.depth=16;
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
    FreeLibrary(module);
    std::cout << "PASS: Windows DLL load; PluginMain; exact Unicode bilingual dialog/PiPL resources; 32/16-bit coordinate paths; 9 tiles each; saved parameters; selection rectangle; padding; cancel; invalid mode/layout.\n";
    std::cout << "NOTE: Mock-host test only. Photoshop loading, real alpha/selection behavior, dialog interaction and AE parity remain unverified.\n";
}
