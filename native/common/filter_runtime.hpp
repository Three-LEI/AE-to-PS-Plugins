#pragma once
#include <windows.h>
#include "PIFilter.h"
#include "psui/plugin_info.hpp"
#include "descriptor_parameters.hpp"
#include <algorithm>
#include <cstring>
namespace psfilter {
template<class Effect> struct Runtime {
static constexpr unsigned long saved_magic=Effect::magic;
struct Saved { unsigned long magic=saved_magic; typename Effect::Parameters values; };
static OSErr read_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>& layers,typename Effect::Parameters& p,bool& interactive){return parameters::read<Effect>(f,layers,p,interactive);}
static OSErr write_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const typename Effect::Parameters& p){return parameters::write<Effect>(f,layers,p);}

static bool read_parameters(FilterRecord* f, Saved& saved) {
    const auto* h=f->handleProcs;
    if(!h || !h->newProc || !h->lockProc || !h->unlockProc || !h->getSizeProc) return false;
    if(!f->parameters) {
        f->parameters=h->newProc(static_cast<int32>(sizeof(Saved)));
        if(!f->parameters) return false;
        auto* p=h->lockProc(f->parameters,true);
        if(!p) return false;
        std::memcpy(p,&saved,sizeof(saved));
        h->unlockProc(f->parameters);
    }
    const auto size=h->getSizeProc(f->parameters);
    const auto* p=h->lockProc(f->parameters,true);
    if(!p) return false;
    if(size==sizeof(Saved))std::memcpy(&saved,p,sizeof(saved));
    else {
        bool upgraded=false;
        if constexpr(requires {Effect::upgrade(p,size,saved.values);})upgraded=Effect::upgrade(p,size,saved.values);
        h->unlockProc(f->parameters);
        if(!upgraded || !Effect::valid(saved.values) || !h->disposeProc)return false;
        auto next=h->newProc(static_cast<int32>(sizeof(Saved)));
        if(!next)return false;
        auto* memory=h->lockProc(next,true);
        if(!memory){h->disposeProc(next);return false;}
        std::memcpy(memory,&saved,sizeof(saved));h->unlockProc(next);
        auto previous=f->parameters;f->parameters=next;h->disposeProc(previous);
        return true;
    }
    h->unlockProc(f->parameters);
    if(saved.magic!=saved_magic) return false;
    return Effect::valid(saved.values);
}
static bool save_parameters(FilterRecord* f,const Saved& saved) {
    auto* p=f->handleProcs->lockProc(f->parameters,true);
    if(!p) return false;
    std::memcpy(p,&saved,sizeof(saved));
    f->handleProcs->unlockProc(f->parameters);
    return true;
}
static VRect full_rect(FilterRecord* f) {
    if(f->bigDocumentData) return f->bigDocumentData->filterRect32;
    return {f->filterRect.top,f->filterRect.left,f->filterRect.bottom,f->filterRect.right};
}
static VRect requested_rect(FilterRecord* f) {
    if(f->bigDocumentData) return f->bigDocumentData->inRect32;
    return {f->inRect.top,f->inRect.left,f->inRect.bottom,f->inRect.right};
}
static void request(FilterRecord* f,VRect r) {
    if(f->bigDocumentData) {
        f->bigDocumentData->inRect32=r; f->bigDocumentData->outRect32=r;
        f->bigDocumentData->maskRect32={};
    } else {
        f->inRect={static_cast<int16>(r.top),static_cast<int16>(r.left),static_cast<int16>(r.bottom),static_cast<int16>(r.right)};
        f->outRect=f->inRect; f->maskRect={};
    }
    f->inLoPlane=f->outLoPlane=0; f->inHiPlane=f->outHiPlane=2;
}
static VRect tile(int32 x,int32 y,VRect all) {
    return {y,x,y+std::min<int32>(64,all.bottom-y),x+std::min<int32>(512,all.right-x)};
}
static int16 process_tile(FilterRecord* f,const Saved& saved) {
    const auto r=requested_rect(f);
    const auto w=r.right-r.left, h=r.bottom-r.top;
    const int bytes=sample_bytes(f);
    if(w<=0 || h<=0 || !f->inData || !f->outData || f->inRowBytes<w*3*bytes || f->outRowBytes<w*3*bytes) return filterBadParameters;
    if((f->inColumnBytes && f->inColumnBytes!=3*bytes) || (f->outColumnBytes && f->outColumnBytes!=3*bytes) ||
       (f->inPlaneBytes && f->inPlaneBytes!=bytes) || (f->outPlaneBytes && f->outPlaneBytes!=bytes)) return filterBadParameters;
    return Effect::render(f,r,saved.values);
}
static bool supported_depth(int depth){if constexpr(requires{Effect::supported_depth(depth);})return Effect::supported_depth(depth);else return depth==8;}
static bool supported_mode(int mode){if constexpr(requires{Effect::supported_mode(mode);})return Effect::supported_mode(mode);else return mode==plugInModeRGBColor;}
static int sample_bytes(FilterRecord* f){if constexpr(requires{Effect::sample_bytes(f);})return Effect::sample_bytes(f);else return 1;}
static void entry(HINSTANCE module_instance,const int16 selector,FilterRecord* f,intptr_t* data,int16* result) {
    if(!result) return;
    *result=noErr;
    if(selector==filterSelectorAbout) {
        psui::show_host_about(Effect::info,Effect::title);
        return;
    }
    if(!f || !data) { *result=filterBadParameters; return; }
    if(f->bigDocumentData) f->bigDocumentData->PluginUsing32BitCoordinates=true;
    switch(selector) {
        case filterSelectorParameters: *data=1; break;
        case filterSelectorPrepare:
            f->bufferSpace=0; f->bufferSpace64=0;
            break;
        case filterSelectorStart: {
            bool show_dialog=*data==1; *data=0;
            if(!supported_mode(f->imageMode) || !supported_depth(f->depth) || f->planes<3) { *result=filterBadMode; break; }
            Saved saved; saved.values=Effect::defaults(f);
            if(!read_parameters(f,saved)) { *result=memFullErr; break; }
            const std::vector<psui::HostLayer> layers;
            if((*result=read_descriptor(f,layers,saved.values,show_dialog)))break;
            if(show_dialog) {
                auto next=saved.values;
                HWND parent=f->platformData ? reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd) : nullptr;
                const auto answer=Effect::dialog(module_instance,parent,f,next);
                if(answer!=IDOK) { *result=answer==-1 ? filterBadParameters : (answer<0 ? static_cast<int16>(answer) : userCanceledErr); break; }
                saved.values=next;
            }
            if((*result=write_descriptor(f,layers,saved.values)))break;
            if(!save_parameters(f,saved)) { *result=memFullErr; break; }
            f->autoMask=true;
            f->wantLayout=0; // SDK traditional layout: rows, columns, planes.
            const auto all=full_rect(f);
            if(all.bottom<=all.top || all.right<=all.left) request(f,{});
            else request(f,tile(all.left,all.top,all));
            break;
        }
        case filterSelectorContinue: {
            if(f->abortProc && f->abortProc()) { *result=userCanceledErr; request(f,{}); break; }
            Saved saved;
            if(!read_parameters(f,saved)) { *result=memFullErr; request(f,{}); break; }
            *result=process_tile(f,saved);
            if(*result!=noErr) { request(f,{}); break; }
            const auto r=requested_rect(f), all=full_rect(f);
            auto x=r.right, y=r.top;
            if(x>=all.right) { x=all.left; y=r.bottom; }
            if(f->progressProc) f->progressProc(y-all.top,all.bottom-all.top);
            if(y>=all.bottom) request(f,{}); else request(f,tile(x,y,all));
            break;
        }
        case filterSelectorFinish: *data=0; request(f,{}); break;
        default: *result=filterBadParameters; break;
    }
}

};
} // namespace psfilter
