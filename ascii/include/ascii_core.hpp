#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
namespace asciifx {
struct Parameters {
 int charset=0,font=0,dither=1,color_mode=0,background=0;
 bool invert=false,bold=false;
 double cell_width=12,cell_aspect=1.6,fill=90,dither_amount=70;
 double brightness=0,contrast=115,gamma=1.2,shift_x=0,shift_y=0,mix=100,color_gain=100;
 std::array<double,3> ink{.85,1,.9},paper{.01,.015,.015};
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
struct Glyph {
 char character=' ';int width=0,height=0;
 double density=0,level=0;std::vector<double> integral;
 void assign(int w,int h,const std::vector<float>& mask);
 double area(double left,double top,double right,double bottom)const;
};
struct GlyphAtlas {
 std::string family;std::vector<Glyph> glyphs;
 void normalize();
 bool valid()const;
};
bool valid(const Parameters&);
Parameters preset(int);
// The Windows adapter draws the installed system font; no font files are bundled.
GlyphAtlas make_atlas(const Parameters&,const Cancel& cancel={});
bool render(const Image&,const Parameters&,const GlyphAtlas&,Image&,const Cancel& cancel={});
}
