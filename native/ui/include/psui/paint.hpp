#pragma once
#include "widgets.hpp"
namespace psui {
// No drawing reaches the target until commit(). Allocation failure preserves
// its previous pixels. A throwing renderer also leaves the last frame intact.
class PaintFrame {
public:
  PaintFrame(HDC target, const RECT&);
  ~PaintFrame();
  PaintFrame(const PaintFrame&)=delete;
  PaintFrame& operator=(const PaintFrame&)=delete;
  HDC dc() const noexcept {return memory_;}
  bool commit();
private:
  HDC target_=nullptr,memory_=nullptr;
  HBITMAP bitmap_=nullptr,old_bitmap_=nullptr;
  RECT rect_{};
};
template<class Render> bool buffered_paint(HDC target,const RECT& rect,const Theme& theme,Render&& render){
  PaintFrame frame(target,rect);
  if(!frame.dc())return false;
  FillRect(frame.dc(),&rect,theme.background());
  render(frame.dc());
  return frame.commit();
}
template<class Render> bool buffered_owner_draw(const DRAWITEMSTRUCT& item,const Theme& theme,Render&& render){
  return buffered_paint(item.hDC,item.rcItem,theme,[&](HDC dc){auto copy=item;copy.hDC=dc;render(copy);});
}
// Theme::apply installs this automatically on all descendant BS_OWNERDRAW /
// SS_OWNERDRAW controls. Call again after adding dynamic children to a live UI.
// Native command, focus and keyboard behavior is retained; only erase is blocked.
bool protect_owner_surfaces(HWND dialog);
}
