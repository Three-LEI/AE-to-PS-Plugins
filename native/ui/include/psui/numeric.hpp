#pragma once
#include "widgets.hpp"
#include <cmath>
#include <cwchar>
namespace psui {
inline bool read_number(HWND w,int id,double lo,double hi,double& out) {
 wchar_t text[64];GetDlgItemTextW(w,id,text,64);wchar_t* end=nullptr;
 const auto n=std::wcstod(text,&end);
 if(!end||end==text||*end||!std::isfinite(n)||n<lo||n>hi)return false;
 out=n;return true;
}
inline void write_number(HWND w,int id,double n) {
 wchar_t text[64];std::swprintf(text,64,L"%.3f",n);
 auto size=std::wcslen(text);while(size&&text[size-1]==L'0')text[--size]=0;if(size&&text[size-1]==L'.')text[--size]=0;
 SetDlgItemTextW(w,id,text);
}
inline void draw_numeric_frame(const DRAWITEMSTRUCT& d,const Theme& t) {
 FillRect(d.hDC,&d.rcItem,t.background());auto b=CreateSolidBrush(t.colors().border);FrameRect(d.hDC,&d.rcItem,b);DeleteObject(b);
}
}
