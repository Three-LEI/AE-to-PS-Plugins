#define WIN32_LEAN_AND_MEAN
#include "psui/widgets.hpp"
#include "psui/scroll_panel.hpp"
#include "psui/paint.hpp"
#include <dwmapi.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <new>
#include <vector>

namespace psui {
Palette palette(int n) noexcept {
    switch(n) {
    case 0:return {RGB(240,240,240),RGB(255,255,255),RGB(228,228,228),RGB(208,208,208),RGB(35,35,35),RGB(120,120,120),RGB(175,175,175),RGB(170,30,30)};
    case 1:return {RGB(184,184,184),RGB(210,210,210),RGB(195,195,195),RGB(165,165,165),RGB(30,30,30),RGB(100,100,100),RGB(135,135,135),RGB(140,15,15)};
    case 2:return {RGB(83,83,83),RGB(65,65,65),RGB(100,100,100),RGB(68,68,68),RGB(238,238,238),RGB(145,145,145),RGB(52,52,52),RGB(255,155,145)};
    default:return {RGB(50,50,50),RGB(38,38,38),RGB(70,70,70),RGB(42,42,42),RGB(230,230,230),RGB(125,125,125),RGB(26,26,26),RGB(255,155,145)};
    }
}
Theme::Theme() { apply(nullptr,3); }
Theme::~Theme() { DeleteObject(background_); DeleteObject(field_); }
bool Theme::apply(HWND w,int level,bool force) {
    level=std::clamp(level,0,3);
    if(level_==level && !force) return false;
    level_=level; colors_=palette(level);
    if(background_) DeleteObject(background_);
    if(field_) DeleteObject(field_);
    background_=CreateSolidBrush(colors_.bg); field_=CreateSolidBrush(colors_.field);
    if(w) {
        protect_owner_surfaces(w);
        const BOOL dark=level>=2;
        DwmSetWindowAttribute(w,20,&dark,sizeof(dark));
        DwmSetWindowAttribute(w,35,&colors_.bg,sizeof(COLORREF));
        DwmSetWindowAttribute(w,36,&colors_.text,sizeof(COLORREF));
        // The dialog paints its background in WM_ERASEBKGND. Invalidation alone
        // leaves the previous theme between child controls.
        RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    }
    return true;
}
INT_PTR Theme::control_color(UINT message,WPARAM wp,bool error) const {
    const auto dc=reinterpret_cast<HDC>(wp);
    const bool edit=message==WM_CTLCOLOREDIT || message==WM_CTLCOLORLISTBOX;
    SetTextColor(dc,error ? colors_.error : colors_.text);
    SetBkColor(dc,edit ? colors_.field : colors_.bg);
    return reinterpret_cast<INT_PTR>(edit ? field_ : background_);
}
void center_on_owner(HWND w,HWND owner) {
    if(owner) owner=GetAncestor(owner,GA_ROOTOWNER);
    MONITORINFO mi{sizeof(mi),{},{},0};
    GetMonitorInfoW(MonitorFromWindow(owner ? owner : w,MONITOR_DEFAULTTONEAREST),&mi);
    RECT area=mi.rcWork,window{};
    if(owner && IsWindowVisible(owner) && !IsIconic(owner)) GetWindowRect(owner,&area);
    GetWindowRect(w,&window);
    const int width=window.right-window.left,height=window.bottom-window.top;
    int x=area.left+(area.right-area.left-width)/2,y=area.top+(area.bottom-area.top-height)/2;
    x=std::clamp<int>(x,mi.rcWork.left,std::max(mi.rcWork.left,mi.rcWork.right-width));
    y=std::clamp<int>(y,mi.rcWork.top,std::max(mi.rcWork.top,mi.rcWork.bottom-height));
    SetWindowPos(w,nullptr,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
void draw_button(const DRAWITEMSTRUCT& d,const Theme& theme,std::optional<COLORREF> swatch) {
    const auto& c=theme.colors(); auto r=d.rcItem;
    FillRect(d.hDC,&r,theme.background());
    const auto brush=CreateSolidBrush(swatch.value_or((d.itemState & ODS_SELECTED) ? c.pressed : c.button));
    const auto pen=CreatePen(PS_SOLID,1,c.border);
    const auto old_brush=SelectObject(d.hDC,brush),old_pen=SelectObject(d.hDC,pen);
    if(swatch) Rectangle(d.hDC,r.left+1,r.top+1,r.right-1,r.bottom-1);
    else RoundRect(d.hDC,r.left+1,r.top+1,r.right-1,r.bottom-1,r.bottom-r.top-2,r.bottom-r.top-2);
    SelectObject(d.hDC,old_pen); SelectObject(d.hDC,old_brush); DeleteObject(pen); DeleteObject(brush);
    if(!swatch) {
        wchar_t text[128]; GetWindowTextW(d.hwndItem,text,128);
        const auto old_font=SelectObject(d.hDC,reinterpret_cast<HFONT>(SendMessageW(d.hwndItem,WM_GETFONT,0,0)));
        SetBkMode(d.hDC,TRANSPARENT); SetTextColor(d.hDC,(d.itemState & ODS_DISABLED) ? c.border : c.text);
        DrawTextW(d.hDC,text,-1,&r,DT_CENTER | DT_VCENTER | DT_SINGLELINE); SelectObject(d.hDC,old_font);
    }
}

void draw_checkbox(const DRAWITEMSTRUCT& d,const Theme& theme,bool checked) {
    const auto c=theme.colors(); auto r=d.rcItem;
    FillRect(d.hDC,&r,theme.background());
    const int size=std::min(15L,r.bottom-r.top-4);
    RECT box{r.left+1,r.top+(r.bottom-r.top-size)/2,r.left+1+size,r.top+(r.bottom-r.top-size)/2+size};
    const auto brush=CreateSolidBrush(c.field);
    const auto pen=CreatePen(PS_SOLID,1,c.border);
    const auto old_brush=SelectObject(d.hDC,brush),old_pen=SelectObject(d.hDC,pen);
    Rectangle(d.hDC,box.left,box.top,box.right,box.bottom);
    if(checked) {
        const auto tick=CreatePen(PS_SOLID,2,c.text); SelectObject(d.hDC,tick);
        MoveToEx(d.hDC,box.left+3,box.top+size/2,nullptr); LineTo(d.hDC,box.left+size/2-1,box.bottom-4); LineTo(d.hDC,box.right-3,box.top+3);
        SelectObject(d.hDC,pen); DeleteObject(tick);
    }
    SelectObject(d.hDC,old_pen); SelectObject(d.hDC,old_brush); DeleteObject(pen); DeleteObject(brush);
    r.left=box.right+5;
    wchar_t text[128]; GetWindowTextW(d.hwndItem,text,128);
    const auto font=SelectObject(d.hDC,reinterpret_cast<HFONT>(SendMessageW(d.hwndItem,WM_GETFONT,0,0)));
    SetTextColor(d.hDC,c.text); SetBkMode(d.hDC,TRANSPARENT);
    DrawTextW(d.hDC,text,-1,&r,DT_LEFT | DT_VCENTER | DT_SINGLELINE); SelectObject(d.hDC,font);
}
void draw_rgb(HDC dc,const RECT& r,const Theme& theme,const std::uint8_t* rgb,int w,int h) {
    FillRect(dc,&r,theme.background());
    const auto border=CreateSolidBrush(theme.colors().border); FrameRect(dc,&r,border); DeleteObject(border);
    if(!rgb || w<=0 || h<=0) {
        auto text=r; SetTextColor(dc,theme.colors().text); SetBkMode(dc,TRANSPARENT);
        DrawTextW(dc,L"无预览 / No preview",-1,&text,DT_CENTER | DT_VCENTER | DT_SINGLELINE); return;
    }
    try {
        std::vector<std::uint32_t> dib(static_cast<std::size_t>(w)*h);
        for(std::size_t i=0;i<dib.size();++i) dib[i]=(rgb[i*3]<<16)|(rgb[i*3+1]<<8)|rgb[i*3+2];
        BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
        const double scale=std::min(1.0,std::min((r.right-r.left-2)/static_cast<double>(w),(r.bottom-r.top-2)/static_cast<double>(h)));
        const int dw=std::max(1,static_cast<int>(w*scale)),dh=std::max(1,static_cast<int>(h*scale));
        const int x=r.left+(r.right-r.left-dw)/2,y=r.top+(r.bottom-r.top-dh)/2;
        StretchDIBits(dc,x,y,dw,dh,0,0,w,h,dib.data(),&info,DIB_RGB_COLORS,SRCCOPY);
    } catch(const std::bad_alloc&) { /* Leave a blank preview if GDI staging fails. */ }
}
namespace {
constexpr UINT bind_message=WM_USER+100,range_message=WM_USER+101,steps_message=WM_USER+102,set_message=WM_USER+103,get_message=WM_USER+104;
struct Range { int lo,hi; };
struct Slider {
    const Theme* theme=nullptr;
    int lo=0,hi=1000,value=0,small=1,page=100;
    bool dragging=false;
    HDC buffer=nullptr;
    HBITMAP bitmap=nullptr,original=nullptr;
    int width=0,height=0;
    ~Slider() { if(buffer) { SelectObject(buffer,original); DeleteObject(bitmap); DeleteDC(buffer); } }
    HDC canvas(HDC target,int w,int h) {
        if(buffer && (width!=w || height!=h)) {
            SelectObject(buffer,original); DeleteObject(bitmap); DeleteDC(buffer); buffer=nullptr;
        }
        if(!buffer) {
            buffer=CreateCompatibleDC(target); bitmap=CreateCompatibleBitmap(target,w,h);
            if(!buffer || !bitmap) { if(bitmap) DeleteObject(bitmap); if(buffer) DeleteDC(buffer); buffer=nullptr; return target; }
            original=static_cast<HBITMAP>(SelectObject(buffer,bitmap)); width=w; height=h;
        }
        return buffer;
    }
};
int padding(const RECT& r) { return std::max(7L,(r.bottom-r.top)/3+2); }
void render_slider(HWND w,Slider& s,HDC target) {
    RECT r; GetClientRect(w,&r);
    if(r.right<=0 || r.bottom<=0) return;
    auto dc=s.canvas(target,r.right,r.bottom);
    const auto c=s.theme ? s.theme->colors() : palette(3);
    const auto bg=CreateSolidBrush(c.bg); FillRect(dc,&r,bg); DeleteObject(bg);
    const int pad=padding(r),span=std::max(1L,r.right-2*pad-1);
    const double fraction=s.hi==s.lo ? 0.0 : (static_cast<double>(s.value)-s.lo)/(static_cast<double>(s.hi)-s.lo);
    const int x=pad+static_cast<int>(std::lround(fraction*span)),y=r.bottom/2;
    const int radius=std::max(3L,r.bottom/4);
    RECT line{pad,y-1,std::max<LONG>(pad+1,r.right-pad),y+1};
    const auto track=CreateSolidBrush(c.track); FillRect(dc,&line,track); DeleteObject(track);
    const auto brush=CreateSolidBrush(c.button); const auto pen=CreatePen(PS_SOLID,1,IsWindowEnabled(w) ? c.text : c.border);
    const auto old_brush=SelectObject(dc,brush),old_pen=SelectObject(dc,pen);
    Ellipse(dc,x-radius,y-radius,x+radius+1,y+radius+1);
    SelectObject(dc,old_pen); SelectObject(dc,old_brush); DeleteObject(pen); DeleteObject(brush);
    if(dc!=target) BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
}
void notify(HWND w,Slider& s,int code) {
    InvalidateRect(w,nullptr,FALSE); // Invalidate the full old + new thumb area.
    SendMessageW(GetParent(w),WM_HSCROLL,MAKEWPARAM(code,s.value),reinterpret_cast<LPARAM>(w));
}
void from_pointer(HWND w,Slider& s,int x,int code) {
    RECT r; GetClientRect(w,&r); const int pad=padding(r);
    const auto t=std::clamp((x-pad)/static_cast<double>(std::max(1L,r.right-2*pad-1)),0.0,1.0);
    s.value=static_cast<int>(std::lround(s.lo+t*(static_cast<double>(s.hi)-s.lo)));
    notify(w,s,code);
}
LRESULT CALLBACK slider_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    auto* s=reinterpret_cast<Slider*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(msg==WM_NCCREATE) { s=new(std::nothrow) Slider; if(!s) return FALSE; SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s)); return TRUE; }
    if(!s) return DefWindowProcW(w,msg,wp,lp);
    switch(msg) {
    case WM_NCDESTROY: SetWindowLongPtrW(w,GWLP_USERDATA,0); delete s; return DefWindowProcW(w,msg,wp,lp);
    case WM_ERASEBKGND: return 1; // No OS/theme erase: every frame paints all pixels.
    case WM_PAINT: { PAINTSTRUCT p; const auto dc=BeginPaint(w,&p); render_slider(w,*s,dc); EndPaint(w,&p); return 0; }
    case WM_PRINTCLIENT: render_slider(w,*s,reinterpret_cast<HDC>(wp)); return 0;
    case WM_SETFOCUS:SendMessageW(GetParent(w),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(w),EN_SETFOCUS),reinterpret_cast<LPARAM>(w));InvalidateRect(w,nullptr,FALSE);return 0;
    case WM_SIZE: case WM_KILLFOCUS: case WM_ENABLE: InvalidateRect(w,nullptr,FALSE); return 0;
    case WM_GETDLGCODE: return DLGC_WANTARROWS;
    case bind_message: s->theme=reinterpret_cast<const Theme*>(lp); InvalidateRect(w,nullptr,FALSE); return 0;
    case range_message: { const auto& range=*reinterpret_cast<const Range*>(lp); s->lo=range.lo; s->hi=std::max(range.lo,range.hi); s->value=std::clamp(s->value,s->lo,s->hi); InvalidateRect(w,nullptr,FALSE); return 0; }
    case steps_message: s->small=std::max(1,static_cast<int>(wp)); s->page=std::max(1,static_cast<int>(lp)); return 0;
    case set_message: {const auto next=std::clamp(static_cast<int>(wp),s->lo,s->hi);if(next!=s->value){s->value=next;InvalidateRect(w,nullptr,FALSE);}return 0;}
    case get_message: return s->value;
    case WM_LBUTTONDOWN: SetFocus(w); s->dragging=true; SetCapture(w); from_pointer(w,*s,GET_X_LPARAM(lp),SB_THUMBTRACK); return 0;
    case WM_MOUSEMOVE: if(s->dragging) from_pointer(w,*s,GET_X_LPARAM(lp),SB_THUMBTRACK); return 0;
    case WM_LBUTTONUP: if(s->dragging) { s->dragging=false; ReleaseCapture(); from_pointer(w,*s,GET_X_LPARAM(lp),SB_THUMBPOSITION); } return 0;
    case WM_CAPTURECHANGED: s->dragging=false; InvalidateRect(w,nullptr,FALSE); return 0;
    case WM_KEYDOWN: {
        long long value=s->value;
        switch(wp) {
        case VK_LEFT: case VK_DOWN: value-=s->small; break;
        case VK_RIGHT: case VK_UP: value+=s->small; break;
        case VK_PRIOR: value+=s->page; break;
        case VK_NEXT: value-=s->page; break;
        case VK_HOME: value=s->lo; break;
        case VK_END: value=s->hi; break;
        default:return DefWindowProcW(w,msg,wp,lp);
        }
        s->value=static_cast<int>(std::clamp<long long>(value,s->lo,s->hi)); notify(w,*s,SB_THUMBPOSITION); return 0;
    }
    default: return DefWindowProcW(w,msg,wp,lp);
    }
}
} // namespace
bool register_widgets(HINSTANCE module) {
    if(!register_scroll_panel(module))return false;
    WNDCLASSW existing{}; if(GetClassInfoW(module,slider_class,&existing)) return true;
    WNDCLASSW c{}; c.lpfnWndProc=slider_proc; c.hInstance=module; c.lpszClassName=slider_class;
    c.hCursor=LoadCursorW(nullptr,IDC_ARROW); c.style=CS_HREDRAW | CS_VREDRAW;
    return RegisterClassW(&c)!=0;
}
void release_widgets(HINSTANCE module) { UnregisterClassW(slider_class,module); release_scroll_panel(module); }
void slider_bind(HWND w,const Theme* t) { SendMessageW(w,bind_message,0,reinterpret_cast<LPARAM>(t)); }
void slider_range(HWND w,int minimum,int maximum) { const Range r{minimum,maximum}; SendMessageW(w,range_message,0,reinterpret_cast<LPARAM>(&r)); }
void slider_steps(HWND w,int small,int page) { SendMessageW(w,steps_message,small,page); }
void slider_set(HWND w,int value) { SendMessageW(w,set_message,value,0); }
int slider_get(HWND w) { return static_cast<int>(SendMessageW(w,get_message,0,0)); }
void StatusLine::bind(HWND dialog,int id,int extra) {
    dialog_=dialog; control_=id; RECT r; GetWindowRect(dialog,&r); base_height_=r.bottom-r.top;
    RECT units{0,0,0,extra}; MapDialogRect(dialog,&units); extra_height_=units.bottom;
    ShowWindow(GetDlgItem(dialog,id),SW_HIDE);
}
void StatusLine::set(const wchar_t* text) {
    const bool show=text && *text;
    SetDlgItemTextW(dialog_,control_,text ? text : L"");
    if(show!=expanded_) {
        expanded_=show;
        RECT r; GetWindowRect(dialog_,&r);
        SetWindowPos(dialog_,nullptr,0,0,r.right-r.left,base_height_+(show ? extra_height_ : 0),SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(GetDlgItem(dialog_,control_),show ? SW_SHOWNA : SW_HIDE);
        InvalidateRect(dialog_,nullptr,TRUE);
    }
}
} // namespace psui
