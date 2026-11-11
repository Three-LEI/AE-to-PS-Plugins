#define WIN32_LEAN_AND_MEAN
#include "psui/paint.hpp"
#include <cwchar>
namespace psui {
PaintFrame::PaintFrame(HDC target,const RECT& rect):target_(target),rect_(rect){
  const int width=rect.right-rect.left,height=rect.bottom-rect.top;
  if(!target||width<=0||height<=0)return;
  memory_=CreateCompatibleDC(target);
  if(!memory_)return;
  bitmap_=CreateCompatibleBitmap(target,width,height);
  if(!bitmap_){DeleteDC(memory_);memory_=nullptr;return;}
  old_bitmap_=static_cast<HBITMAP>(SelectObject(memory_,bitmap_));
  SetViewportOrgEx(memory_,-rect.left,-rect.top,nullptr);
  SelectObject(memory_,GetCurrentObject(target,OBJ_FONT));
}
PaintFrame::~PaintFrame(){if(memory_){SelectObject(memory_,old_bitmap_);DeleteObject(bitmap_);DeleteDC(memory_);}}
bool PaintFrame::commit(){return memory_&&BitBlt(target_,rect_.left,rect_.top,rect_.right-rect_.left,rect_.bottom-rect_.top,memory_,rect_.left,rect_.top,SRCCOPY)!=FALSE;}
namespace {
constexpr auto key=L"ZaYu.PSUI.OwnerEraseGuard.1";
LRESULT CALLBACK guard(HWND w,UINT msg,WPARAM wp,LPARAM lp){
  const auto original=reinterpret_cast<WNDPROC>(GetPropW(w,key));
  if(!original)return DefWindowProcW(w,msg,wp,lp);
  if(msg==WM_ERASEBKGND)return 1;
  if(msg==WM_NCDESTROY){RemovePropW(w,key);SetWindowLongPtrW(w,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(original));}
  return CallWindowProcW(original,w,msg,wp,lp);
}
BOOL CALLBACK bind(HWND w,LPARAM data){
  wchar_t cls[32];GetClassNameW(w,cls,32);const auto style=GetWindowLongPtrW(w,GWL_STYLE);
  const bool owner=(!_wcsicmp(cls,L"Button")&&(style&BS_TYPEMASK)==BS_OWNERDRAW)||(!_wcsicmp(cls,L"Static")&&(style&SS_TYPEMASK)==SS_OWNERDRAW);
  if(!owner||GetPropW(w,key))return TRUE;
  const auto original=GetWindowLongPtrW(w,GWLP_WNDPROC);
  if(!SetPropW(w,key,reinterpret_cast<HANDLE>(original))){*reinterpret_cast<bool*>(data)=false;return FALSE;}
  SetWindowLongPtrW(w,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(guard));
  return TRUE;
}
}
bool protect_owner_surfaces(HWND dialog){bool result=true;EnumChildWindows(dialog,bind,reinterpret_cast<LPARAM>(&result));return result;}
}
