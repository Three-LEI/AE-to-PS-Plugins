#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "tint_dialog.hpp"
#include "psui/widgets.hpp"
#include "psui/paint.hpp"
#include "psui/numeric.hpp"
#include "psui/photoshop_host.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <new>
#include <vector>

namespace tint { namespace {
struct Context {
    Parameters values;
    FilterRecord* host;
    psui::Theme theme;
    psui::StatusLine status;
    psui::ProxyImage source;
    std::vector<std::uint8_t> processed,display;
    OSErr error=noErr;
    bool updating=false,picking=false,preview=true,preview_pending=false;
    Context(const Parameters& p,FilterRecord* f):values(p),host(f) {}
};
bool read_amount(HWND w,double& amount) {
    double value;
    if(!psui::read_number(w,203,0,100,value))return false;
    amount=value/100; return true;
}
void amount_text(HWND w,double amount) {
    psui::write_number(w,203,amount*100);
}
void queue_preview(HWND w,Context& c) {
    // Throttle instead of restarting a debounce timer on every mouse move:
    // continuous dragging must still produce preview frames.
    if(!c.preview_pending) c.preview_pending=SetTimer(w,2,25,nullptr)!=0;
}
void update_preview(HWND w,Context& c) {
    if(c.source.pixels.empty()) return;
    const auto& src=c.source;
    ConstImage input{src.pixels.data(),src.pixels.size(),static_cast<std::size_t>(src.width),static_cast<std::size_t>(src.height),static_cast<std::size_t>(src.width*src.channels),static_cast<unsigned>(src.channels),SampleType::u8};
    Image output{c.processed.data(),c.processed.size(),input.width,input.height,input.row_bytes,input.components,SampleType::u8};
    if(c.preview) process(input,output,c.values);
    for(int y=0;y<src.height;++y) for(int x=0;x<src.width;++x) {
        const auto i=static_cast<std::size_t>(y*src.width+x),p=i*src.channels;
        const int a=src.channels==4 ? src.pixels[p+3] : 255;
        const int m=src.mask.empty() ? 255 : src.mask[i];
        const int checker=((x/8+y/8)&1) ? 156 : 200;
        for(int ch=0;ch<3;++ch) {
            const int original=src.pixels[p+ch];
            const int changed=c.preview ? (original*(255-m)+c.processed[p+ch]*m+127)/255 : original;
            c.display[i*3+ch]=static_cast<std::uint8_t>((changed*a+checker*(255-a)+127)/255);
        }
    }
    InvalidateRect(GetDlgItem(w,210),nullptr,FALSE);
}
void fields(HWND w,Context& c) {
    c.updating=true; amount_text(w,c.values.amount);
    psui::slider_set(GetDlgItem(w,207),static_cast<int>(std::lround(c.values.amount*1000))); c.updating=false;
    c.status.set(L""); InvalidateRect(GetDlgItem(w,205),nullptr,FALSE); InvalidateRect(GetDlgItem(w,206),nullptr,FALSE); queue_preview(w,c);
}
void draw_item(HWND,const DRAWITEMSTRUCT& d,Context& c) {
    if(d.CtlID==220) {
        psui::draw_numeric_frame(d,c.theme); return;
    }
    if(d.CtlID==210) {
        // The host callback provides Photoshop's display conversion when present.
        if(c.display.empty()) psui::draw_rgb(d.hDC,d.rcItem,c.theme,nullptr,0,0);
        else {
            FillRect(d.hDC,&d.rcItem,c.theme.background());
            const auto border=CreateSolidBrush(c.theme.colors().border); FrameRect(d.hDC,&d.rcItem,border); DeleteObject(border);
            if(!psui::host_display_rgb(c.host,d.hDC,d.rcItem,c.display.data(),c.source.width,c.source.height))
                psui::draw_rgb(d.hDC,d.rcItem,c.theme,c.display.data(),c.source.width,c.source.height);
        }
        return;
    }
    if(d.CtlID==217) { psui::draw_checkbox(d,c.theme,c.preview); return; }
    if(d.CtlID==205 || d.CtlID==206) {
        const auto* color=d.CtlID==205 ? c.values.black : c.values.white;
        psui::draw_button(d,c.theme,RGB(static_cast<int>(color[0]*255+0.5),static_cast<int>(color[1]*255+0.5),static_cast<int>(color[2]*255+0.5)));
    } else psui::draw_button(d,c.theme);
}
INT_PTR CALLBACK dialog_proc(HWND w,UINT message,WPARAM wp,LPARAM lp) {
    auto* c=reinterpret_cast<Context*>(GetWindowLongPtrW(w,DWLP_USER));
    if(message==WM_INITDIALOG) {
        c=reinterpret_cast<Context*>(lp); SetWindowLongPtrW(w,DWLP_USER,lp);
        c->status.bind(w,204,32);
        psui::slider_bind(GetDlgItem(w,207),&c->theme); psui::slider_range(GetDlgItem(w,207),0,1000); psui::slider_steps(GetDlgItem(w,207),1,100);
        SendDlgItemMessageW(w,203,EM_SETLIMITTEXT,10,0);
        c->theme.apply(w,psui::host_brightness(c->host),true); fields(w,*c);
        RECT r; GetClientRect(GetDlgItem(w,210),&r);
        c->error=psui::host_read_proxy(c->host,std::min(r.right,r.bottom)-6,c->source);
        if(c->error!=noErr) { EndDialog(w,IDCANCEL); return TRUE; }
        try { c->processed.resize(c->source.pixels.size()); c->display.resize(static_cast<std::size_t>(c->source.width)*c->source.height*3); }
        catch(const std::bad_alloc&) { c->error=memFullErr; EndDialog(w,IDCANCEL); return TRUE; }
        EnableWindow(GetDlgItem(w,217),!c->source.pixels.empty()); update_preview(w,*c);
        psui::center_on_owner(w,GetWindow(w,GW_OWNER)); SetTimer(w,1,750,nullptr); return TRUE;
    }
    if(!c) return FALSE;
    if(message==WM_TIMER) {
        if(wp==1 && !c->picking) c->theme.apply(w,psui::host_brightness(c->host));
        if(wp==2) { KillTimer(w,2); c->preview_pending=false; update_preview(w,*c); } return TRUE;
    }
    if(message==WM_THEMECHANGED || message==WM_SETTINGCHANGE) { c->theme.apply(w,psui::host_brightness(c->host),true); return TRUE; }
    if(message==WM_DESTROY) { KillTimer(w,1); KillTimer(w,2); return TRUE; }
    if(message==WM_ERASEBKGND) { RECT r; GetClientRect(w,&r); FillRect(reinterpret_cast<HDC>(wp),&r,c->theme.background()); return TRUE; }
    if(message==WM_CTLCOLORDLG || message==WM_CTLCOLORSTATIC || message==WM_CTLCOLOREDIT || message==WM_CTLCOLORBTN || message==WM_CTLCOLORLISTBOX)
        return c->theme.control_color(message,wp,GetDlgCtrlID(reinterpret_cast<HWND>(lp))==204);
    if(message==WM_DRAWITEM) { psui::buffered_owner_draw(*reinterpret_cast<DRAWITEMSTRUCT*>(lp),c->theme,[&](const DRAWITEMSTRUCT& d){draw_item(w,d,*c);}); return TRUE; }
    if(message==WM_HSCROLL && reinterpret_cast<HWND>(lp)==GetDlgItem(w,207)) {
        c->values.amount=psui::slider_get(GetDlgItem(w,207))/1000.0; c->updating=true; amount_text(w,c->values.amount); c->updating=false;
        c->status.set(L""); queue_preview(w,*c); return TRUE;
    }
    if(message==WM_COMMAND) {
        const auto id=LOWORD(wp),code=HIWORD(wp);
        if(id==IDCANCEL) { EndDialog(w,IDCANCEL); return TRUE; }
        if(code==EN_CHANGE && !c->updating) {
            if(id==203 && read_amount(w,c->values.amount)) psui::slider_set(GetDlgItem(w,207),static_cast<int>(std::lround(c->values.amount*1000)));
            c->status.set(L""); queue_preview(w,*c); return TRUE;
        }
        if(id==217) { c->preview=!c->preview; InvalidateRect(GetDlgItem(w,217),nullptr,FALSE); update_preview(w,*c); return TRUE; }
        if(id==205 || id==206) {
            auto* color=id==205 ? c->values.black : c->values.white;
            c->picking=true; const auto err=psui::host_pick_rgb(c->host,color); c->picking=false;
            if(err==noErr) fields(w,*c); else if(err!=userCanceledErr) c->status.set(L"无法打开选色器 / Color picker unavailable."); return TRUE;
        }
        if(id==209) { c->values=Parameters{}; fields(w,*c); return TRUE; }
        if(id==208 || id==IDOK) {
            auto next=c->values;
            if(!read_amount(w,next.amount)) { c->status.set(L"着色量需为 0–100%。\nEnter an amount from 0 to 100%."); return TRUE; }
            if(id==208) for(int i=0;i<3;++i) std::swap(next.black[i],next.white[i]); c->values=next;
            if(id==IDOK) EndDialog(w,IDOK); else fields(w,*c); return TRUE;
        }
    }
    return FALSE;
}
} // namespace
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,Parameters& values) {
    if(!psui::register_widgets(module)) return -1;
    Context c(values,host);
    const auto result=DialogBoxParamW(module,MAKEINTRESOURCEW(101),owner,dialog_proc,reinterpret_cast<LPARAM>(&c));
    psui::release_widgets(module);
    if(c.error!=noErr) return c.error;
    if(result==IDOK) values=c.values; return result;
}
} // namespace tint
