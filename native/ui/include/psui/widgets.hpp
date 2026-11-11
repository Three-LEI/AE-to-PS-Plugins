#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <optional>
#include <cstdint>

namespace psui {
inline constexpr wchar_t slider_class[]=L"ZaYu.PSUI.Slider.1";
struct Palette { COLORREF bg,field,button,pressed,text,border,track,error; };
Palette palette(int level) noexcept;
class Theme {
public:
    Theme();
    ~Theme();
    Theme(const Theme&)=delete;
    Theme& operator=(const Theme&)=delete;
    bool apply(HWND dialog,int level,bool force=false);
    const Palette& colors() const noexcept { return colors_; }
    HBRUSH background() const noexcept { return background_; }
    HBRUSH field() const noexcept { return field_; }
    INT_PTR control_color(UINT message,WPARAM dc,bool error=false) const;
private:
    int level_=-1;
    Palette colors_{};
    HBRUSH background_=nullptr,field_=nullptr;
};
void center_on_owner(HWND window,HWND owner);
void draw_button(const DRAWITEMSTRUCT&,const Theme&,std::optional<COLORREF> swatch={});
void draw_checkbox(const DRAWITEMSTRUCT&,const Theme&,bool checked);
void draw_rgb(HDC,const RECT&,const Theme&,const std::uint8_t* rgb,int width,int height);

// Register before creating a dialog resource containing slider_class; unregister
// after all those dialogs close and before unloading a plug-in DLL.
bool register_widgets(HINSTANCE module);
void release_widgets(HINSTANCE module);
void slider_bind(HWND slider,const Theme* theme);
void slider_range(HWND slider,int minimum,int maximum);
void slider_steps(HWND slider,int small,int page);
void slider_set(HWND slider,int value);
int slider_get(HWND slider);

// A status row consumes space only when there is a validation message.
class StatusLine {
public:
    void bind(HWND dialog,int control,int extra_height_dlu=26);
    void set(const wchar_t* text);
private:
    HWND dialog_=nullptr;
    int control_=0,base_height_=0,extra_height_=0;
    bool expanded_=false;
};
} // namespace psui
