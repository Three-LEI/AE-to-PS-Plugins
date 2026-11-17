#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "ascii_core.hpp"
#include <algorithm>
#include <cstring>
#include <cwchar>
namespace asciifx {namespace {
struct Canvas {
 HDC dc=CreateCompatibleDC(nullptr);HBITMAP bitmap=nullptr;HFONT font=nullptr;HGDIOBJ old_bitmap=nullptr,old_font=nullptr;void* pixels=nullptr;
 ~Canvas(){if(dc){if(old_font)SelectObject(dc,old_font);if(old_bitmap)SelectObject(dc,old_bitmap);}if(font)DeleteObject(font);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);}
};
}
GlyphAtlas make_atlas(const Parameters& p,const Cancel& cancel){
 if(!valid(p)||(cancel&&cancel()))return {};
 constexpr int w=64,h=96;constexpr const wchar_t* fonts[]={L"Consolas",L"Courier New",L"Lucida Console"};
 constexpr const char* sets[]={" .:-=+*#%@"," .,:;!iIl1tfrxvYZJCLQ0Oqpdbkhao*MW&8%B@$#"," 01"," 0123456789"};
 Canvas c;if(!c.dc)return {};BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 c.bitmap=CreateDIBSection(c.dc,&info,DIB_RGB_COLORS,&c.pixels,nullptr,0);if(!c.bitmap||!c.pixels)return {};c.old_bitmap=SelectObject(c.dc,c.bitmap);
 auto choose=[&](const wchar_t* face){return CreateFontW(-88,58,0,0,p.bold?FW_BOLD:FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH|FF_MODERN,face);};
 c.font=choose(fonts[p.font]);if(!c.font)return {};c.old_font=SelectObject(c.dc,c.font);wchar_t family[128]{};if(!GetTextFaceW(c.dc,128,family))return {};
 if(std::wcscmp(family,fonts[p.font])!=0){SelectObject(c.dc,c.old_font);DeleteObject(c.font);c.font=choose(L"Courier New");if(!c.font)return {};SelectObject(c.dc,c.font);if(!GetTextFaceW(c.dc,128,family))return {};}
 SetBkColor(c.dc,RGB(0,0,0));SetBkMode(c.dc,OPAQUE);SetTextColor(c.dc,RGB(255,255,255));TEXTMETRICW metrics{};if(!GetTextMetricsW(c.dc,&metrics))return {};
 GlyphAtlas atlas;char utf8[512]{};if(!WideCharToMultiByte(CP_UTF8,0,family,-1,utf8,512,nullptr,nullptr))return {};atlas.family=utf8;
 const RECT bounds{0,0,w,h};std::vector<float> mask(w*h);
 for(const char* at=sets[p.charset];*at;at++){
  if(cancel&&cancel())return {};const wchar_t ch=static_cast<unsigned char>(*at);SIZE size{};if(!GetTextExtentPoint32W(c.dc,&ch,1,&size))return {};
  if(!ExtTextOutW(c.dc,(w-size.cx)/2,(h-metrics.tmHeight)/2,ETO_OPAQUE|ETO_CLIPPED,&bounds,&ch,1,nullptr)||!GdiFlush())return {};
  const auto* bytes=static_cast<const unsigned char*>(c.pixels);for(std::size_t i=0;i<mask.size();i++)mask[i]=bytes[i*4+2]/255.f;
  Glyph glyph;glyph.character=*at;glyph.assign(w,h,mask);atlas.glyphs.push_back(std::move(glyph));
 }
 atlas.normalize();return atlas.valid()?std::move(atlas):GlyphAtlas{};
}
}
