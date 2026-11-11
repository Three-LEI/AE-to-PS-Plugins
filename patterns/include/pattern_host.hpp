#pragma once
#include "pattern_core.hpp"
#include "psui/plugin_info.hpp"
#include "psui/host_layers.hpp"
#ifdef ZAYU_CHECKERBOARD
inline constexpr patterns::Kind pattern_kind=patterns::Kind::checker;
inline constexpr psui::PluginInfo pattern_info{"Checkerboard 棋盘","0.6"};
inline constexpr auto pattern_title=L"Checkerboard 棋盘";
#else
inline constexpr patterns::Kind pattern_kind=patterns::Kind::grid;
inline constexpr psui::PluginInfo pattern_info{"Grid 网格","0.6"};
inline constexpr auto pattern_title=L"Grid 网格";
#endif
namespace patterns {
inline VRect document_bounds(FilterRecord* f){if(f->documentInfo)return f->documentInfo->bounds;return {0,0,f->bigDocumentData?f->bigDocumentData->imageSize32.v:f->imageSize.v,f->bigDocumentData?f->bigDocumentData->imageSize32.h:f->imageSize.h};}
inline Parameters defaults_for_host(FilterRecord* f){const auto r=document_bounds(f);return defaults(pattern_kind,r.right-r.left,r.bottom-r.top,r.left,r.top);}
inline bool locked_alpha(FilterRecord* f){return f->filterCase==filterCaseProtectedTransparencyNoSelection||f->filterCase==filterCaseProtectedTransparencyWithSelection;}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);
}
