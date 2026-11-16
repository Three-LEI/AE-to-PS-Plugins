#include "energy_core.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <limits>
namespace {
int checks=0;
void require(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
void record(std::vector<std::uint8_t>& b,int type,int count=0){b.resize(b.size()+26);auto* p=b.data()+b.size()-26;p[0]=std::uint8_t(type>>8);p[1]=std::uint8_t(type);p[2]=std::uint8_t(count>>8);p[3]=std::uint8_t(count);}
void point(std::vector<std::uint8_t>& b,int at,double x,double y){auto* p=b.data()+b.size()-26+at;for(auto d:{y,x}){auto v=std::uint32_t(std::int32_t(std::lround(d*16777216)));for(int j=3;j>=0;j--)*p++=std::uint8_t(v>>(j*8));}}
double alpha(const energy::Image& i,int x,int y){return i.rgba[(std::size_t(y)*i.width+x)*4+3];}
void ppm(const std::filesystem::path& path,const energy::Image& image){std::ofstream f(path,std::ios::binary);f<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";for(std::size_t n=0;n<image.rgba.size();n+=4)for(int k=0;k<3;k++){auto v=std::uint8_t(std::lround(std::clamp(image.rgba[n+k]*image.rgba[n+3],0.f,1.f)*255));f.write(reinterpret_cast<const char*>(&v),1);}}
}
int main(int argc,char** argv){try{
 using namespace energy;const auto dir=argc>1?std::filesystem::path(argv[1]):std::filesystem::path(".");std::filesystem::create_directories(dir);
 std::vector<std::uint8_t> b;record(b,6);record(b,3,2);record(b,5);point(b,2,.1,.5);point(b,10,.1,.5);point(b,18,.3,.1);record(b,5);point(b,2,.7,.1);point(b,10,.9,.5);point(b,18,.9,.5);
 Path curve;std::string why;require(parse_path(b,400,240,curve,why),"read Adobe records");require(curve.curves[0].knots[0].anchor==Point{40.000009536743164,120},"Q8.24 horizontal/vertical order");require(std::abs(curve.curves[0].knots[1].before.y-24)<1e-4,"incoming control retained");
 auto keep=curve;for(std::size_t n=0;n<b.size();n++){auto short_data=std::vector<std::uint8_t>(b.begin(),b.begin()+n);const bool ok=parse_path(short_data,400,240,curve,why);require(!ok,"reject truncated resources");require(curve==keep,"parser failure transactional");}
 auto bad=b;bad[26+3]=250;require(!parse_path(bad,400,240,curve,why),"reject bad count");bad=b;bad[52+1]=2;require(!parse_path(bad,400,240,curve,why),"reject mixed closed knot");bad=b;bad.push_back(0);require(!parse_path(bad,400,240,curve,why),"reject partial tail");require(!parse_path(b,-1,240,curve,why),"reject invalid dimensions");
 auto snap=encode_path(curve);Path restored;require(decode_path(snap,restored)&&restored==curve,"geometry snapshot exact roundtrip");for(std::size_t n=0;n<snap.size();n++){auto short_data=std::vector<std::uint8_t>(snap.begin(),snap.begin()+n);require(!decode_path(short_data,restored),"snapshot truncation");require(restored==curve,"snapshot failure transactional");}bad=snap;bad.push_back(0);require(!decode_path(bad,restored),"snapshot trailing bytes");
 std::vector<Segment> segments;require(flatten(curve,.1,segments)&&segments.size()>10,"adaptive curved flattening");require(segments.front().u0==0&&segments.back().u1==1,"arc-length range");double previous=0;for(const auto& s:segments){require(s.u0==previous&&s.u1>=s.u0,"monotonic arc length");previous=s.u1;}
 Image src{400,240,0,0,std::vector<float>(400*240*4)},out;auto p=defaults(400,240);p.composite=1;p.glow_radius=24;require(render(src,p,{},out),"two-point output");require(alpha(out,200,120)>.9&&alpha(out,200,130)>0&&alpha(out,10,10)==0,"white core, colored halo, empty far field");ppm(dir/"two-points.ppm",out);
 p.input=1;p.trim_start=0;p.trim_end=100;require(render(src,p,curve,out),"Bezier path output");ppm(dir/"pen-path.ppm",out);require(alpha(out,200,48)>.8&&alpha(out,200,120)<.01,"curve uses control points");
 p.glow_radius=0;p.trim_start=25;p.trim_end=75;require(render(src,p,curve,out),"trim path");require(alpha(out,40,120)==0&&alpha(out,200,48)>.8,"trim removes ends");p.trim_end=p.trim_start;require(render(src,p,curve,out),"empty trim");require(std::all_of(out.rgba.begin(),out.rgba.end(),[](float v){return v==0;}),"empty range no light");
 for(std::size_t n=0;n<src.rgba.size();n+=4){src.rgba[n]=2.5f;src.rgba[n+1]=.3f;src.rgba[n+2]=.1f;src.rgba[n+3]=.25f;}p.composite=0;require(render(src,p,curve,out)&&out.rgba==src.rgba,"empty trim exactly preserves HDR/alpha");p.trim_end=100;p.brightness=0;require(render(src,p,curve,out)&&out.rgba==src.rgba,"brightness zero identity");p.brightness=1;p.trim_start=0;require(render(src,p,curve,out),"HDR render");for(auto v:out.rgba)require(std::isfinite(v)&&v>=0,"finite native HDR values");require(out.rgba[0]>1,"HDR retained");
 auto untouched=out;int calls=0;require(!render(src,p,curve,out,[&]{return ++calls>4;})&&out.rgba==untouched.rgba,"cancel leaves output untouched");auto malformed=p;malformed.width=std::numeric_limits<double>::quiet_NaN();require(!valid(malformed)&&!render(src,malformed,curve,out),"reject nonfinite parameters");
 p.input=0;p.start_x=80;p.end_x=320;p.start_y=p.end_y=120;p.composite=1;p.glow_radius=20;Image a;require(render(src,p,{},a),"offset baseline");auto moved=p;moved.offset_x=13;moved.offset_y=7;Image shifted;require(render(src,moved,{},shifted),"offset render");for(int y=30;y<200;y++)for(int x=40;x<340;x++)require(std::abs(alpha(a,x,y)-alpha(shifted,x+13,y+7))<1e-6,"translation invariant");
 auto q=draft_parameters(p,20,10,4);require(q.start_x==15&&q.start_y==27.5&&q.width==.75&&q.glow_radius==5,"draft scales positions and physical lengths only");auto dp=draft_path(curve,20,10,4);require(std::abs(dp.curves[0].knots[0].anchor.x-5)<1e-4,"draft path transform");
 // Closed subpaths, isolated points and collinear overshooting Beziers.
 auto closed=line_path(p);closed.curves[0].closed=true;require(flatten(closed,.2,segments)&&segments.size()==2,"closed path includes closing segment");closed.curves[0].knots.resize(1);require(flatten(closed,.2,segments)&&segments.size()==1,"isolated knot");auto loop=line_path(p);loop.curves[0].knots[0].after={600,120};loop.curves[0].knots[1].before={-100,120};require(flatten(loop,.2,segments)&&segments.size()>1,"collinear overshoot not collapsed");
 std::ofstream result(dir/"results.txt");result<<"PASS "<<checks<<" core checks\n";std::cout<<"PASS "<<checks<<" energy core checks\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
