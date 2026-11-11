// Development-only native host probe. Never included in a release package.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "PIFilter.h"
#include "PIProperties.h"
#include "psui/localization.hpp"
#include "psui/photoshop_host.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifndef AEPS_LOCALE_QA_OUTPUT
#error Define a local report path when building the locale probe
#endif
namespace {
std::string read(FilterRecord* host,bool simple_pointer) {
    std::ostringstream out;
    if(!host||!host->propertyProcs||!host->propertyProcs->getPropertyProc||!host->handleProcs)return "missing callbacks";
    auto& h=*host->handleProcs;
    if(!h.getSizeProc||!h.lockProc||!h.unlockProc||!h.disposeProc)return "incomplete handle callbacks";
    Handle handle=nullptr;intptr_t simple=0;
    const auto err=host->propertyProcs->getPropertyProc(0x3842494d,propLocale,0,simple_pointer?&simple:nullptr,&handle);
    out<<"status="<<err<<" simple="<<simple<<" handle="<<(handle!=nullptr);
    if(handle){
        const auto size=h.getSizeProc(handle);out<<" size="<<size;
        if(size>0&&size<=128){
            auto* bytes=h.lockProc(handle,false);
            if(bytes){
                out<<" decoded="<<psui::decode_host_locale({bytes,static_cast<std::size_t>(size)})<<" hex=";
                for(int i=0;i<size;++i)out<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<unsigned>(static_cast<unsigned char>(bytes[i]));
                h.unlockProc(handle);
            }
        }
        h.disposeProc(handle);
    }
    return out.str();
}
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* host,intptr_t*,int16* result) {
    *result=noErr;
    if(selector==filterSelectorParameters||selector==filterSelectorPrepare||selector==filterSelectorStart){
        std::ofstream out(std::filesystem::path(AEPS_LOCALE_QA_OUTPUT),std::ios::app);
        out<<"selector="<<selector<<"\nnull_simple "<<read(host,false)<<"\nvalid_simple "<<read(host,true)<<"\nresolved="<<psui::host_locale(host)<<"\n";
        if(selector!=filterSelectorPrepare)*result=userCanceledErr;
    }
}
