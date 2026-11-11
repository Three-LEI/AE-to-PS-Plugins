#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <map>

// Inspect the resource actually presented to old Photoshop hosts.
inline std::map<std::string,std::string> legacy_pipl(HMODULE module) {
    auto resource=FindResourceW(module,MAKEINTRESOURCEW(16000),L"PIPL");
    if(!resource) throw std::runtime_error("legacy PiPL resource missing");
    auto memory=LoadResource(module,resource);
    if(!memory) throw std::runtime_error("legacy PiPL load failed");
    const std::string bytes(static_cast<const char*>(LockResource(memory)),SizeofResource(module,resource));
    auto word=[&](std::size_t at){
        if(at+4>bytes.size()) throw std::runtime_error("truncated PiPL");
        std::uint32_t value;std::memcpy(&value,bytes.data()+at,4);return value;
    };
    if(bytes.size()<10 || bytes[0]!=1 || bytes[1]!=0 || word(2)!=0) throw std::runtime_error("invalid PiPL header");
    std::map<std::string,std::string> properties;std::size_t at=10;
    for(std::uint32_t i=0,count=word(6);i<count;++i){
        const auto length=word(at+12);
        if(bytes.substr(at,4)!="MIB8" || at+16+length>bytes.size()) throw std::runtime_error("invalid PiPL property");
        auto key=bytes.substr(at+4,4);std::reverse(key.begin(),key.end());
        properties.emplace(key,bytes.substr(at+16,length));at+=16+length;
    }
    if(at!=bytes.size()) throw std::runtime_error("PiPL property size mismatch");
    return properties;
}
