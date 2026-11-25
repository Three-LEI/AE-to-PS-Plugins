#include "psui/adaptive_preview.hpp"
#include "glow_core.hpp"
#include "chromatic_core.hpp"
#include "vector_core.hpp"
#include "lens_core.hpp"
#include "jpeg_core.hpp"
#include "surface_core.hpp"
#include "creative_core.hpp"
#include "roughen_core.hpp"
#include "lightfx_core.hpp"
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>
#include <stdexcept>
namespace glow {
#include "../glow/src/preview_adapter.hpp"
}
namespace chromatic {
#include "../chromatic/src/preview_adapter.hpp"
}
namespace vectorblur {
#include "../vector/src/preview_adapter.hpp"
}
namespace lens {
#include "../lens/src/preview_adapter.hpp"
}
namespace jpegfx {
#include "../jpeg/src/preview_adapter.hpp"
}
namespace surface {
#include "../surface/src/preview_adapter.hpp"
}
namespace creative {
#include "../creative/src/preview_adapter.hpp"
}
namespace roughen {
#include "../roughen/src/preview_adapter.hpp"
}
int checks=0;void check(bool condition,const char* message){++checks;if(!condition)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 psui::PreviewSpace space{-117,43,8};
 check(space.x(-117)==0&&space.y(43)==0&&space.length(80)==10,"native coordinate conversion");
 glow::Image image{17,13,-117,43,std::vector<float>(17*13*4)};
 for(int y=0;y<13;y++)for(int x=0;x<17;x++){auto i=(y*17+x)*4;image.rgba[i]=x;image.rgba[i+1]=y;image.rgba[i+2]=.5f;image.rgba[i+3]=(x%3)*.5f;}
 psui::DraftImageCache<glow::Image> cache;auto no=[]{return false;};
 check(&cache.get(image,{-117,43,1},no)==&image,"full preview receives original snapshot");
 auto& small=cache.get(image,space,no);check(small.valid()&&small.width==3&&small.height==2&&small.left==0,"draft bounded and aligned");
 double original[4]{},reduced[4]{};
 for(std::size_t i=0;i<image.rgba.size();i+=4){for(int k=0;k<3;k++)original[k]+=image.rgba[i+k]*image.rgba[i+3];original[3]+=image.rgba[i+3];}
 for(int y=0;y<small.height;y++)for(int x=0;x<small.width;x++){auto i=(y*small.width+x)*4;double area=std::min(8,17-x*8)*std::min(8,13-y*8);for(int k=0;k<3;k++)reduced[k]+=small.rgba[i+k]*small.rgba[i+3]*area;reduced[3]+=small.rgba[i+3]*area;}
 for(int k=0;k<4;k++)check(std::abs(original[k]-reduced[k])<.001,"draft conserves premultiplied energy and coverage");
 auto reference=image;reference.left+=3;reference.top-=6;const auto& aligned=cache.get(reference,space,no);check(aligned.left==0&&aligned.top==-1&&aligned.height==2,"reference layers share target grid");
 cache.clear();check(!cache.get(image,space,[]{return true;}).valid(),"cancelled draft not cached");check(cache.get(image,space,no).valid(),"retry after cancellation valid");
 for(double step:{2.,8.,512.}){
  space.step=step;
  {glow::Parameters p;auto q=glow::draft_parameters(p,space);check(glow::valid(q)&&p.radius==glow::Parameters{}.radius,"Glow draft parameters valid and original intact");}
  {chromatic::Parameters p;check(chromatic::valid(chromatic::draft_parameters(p,space)),"Dispersion draft valid");}
  {vectorblur::Parameters p;check(vectorblur::valid(vectorblur::draft_parameters(p,space)),"Vector draft valid");}
  {lens::Parameters p;check(lens::valid(lens::draft_parameters(p,space)),"Lens draft valid");}
  {jpegfx::Parameters p;check(jpegfx::valid(jpegfx::draft_parameters(p,space)),"JPEG draft valid");}
  {surface::Parameters p;check(surface::valid(surface::draft_parameters(p,space)),"Surface draft valid");}
  {creative::Parameters p;check(creative::valid(creative::draft_parameters(p,space)),"Creative draft valid");}
  {roughen::Parameters p;check(roughen::valid(roughen::draft_parameters(p,space)),"Roughen draft valid");}
 }
 psui::AdaptivePreview<int> worker;bool dirty=true;int result=-1,seen=0;double submit_ms=0;
 auto job=[](bool draft,const auto& cancel){for(int i=0;i<(draft?4:30);i++){if(cancel())return -10;std::this_thread::sleep_for(std::chrono::milliseconds(1));}return draft?1:2;};
 for(int i=0;i<45;i++){dirty=true;auto t=std::chrono::steady_clock::now();if(worker.update(dirty,result,job)){check(result==1,"drag displays drafts");++seen;}submit_ms=std::max(submit_ms,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());std::this_thread::sleep_for(std::chrono::milliseconds(8));}
 for(int i=0;i<150&&worker.pending();i++){worker.update(dirty,result,job);std::this_thread::sleep_for(std::chrono::milliseconds(5));}
 worker.update(dirty,result,job);check(seen>=10&&submit_ms<100,"continuous drag publishes frames without joining");check(result==2&&worker.full_frames>=1&&!worker.pending(),"idle refines to full quality");
 worker.fast=false;dirty=true;worker.update(dirty,result,job);for(int i=0;i<50&&worker.pending();i++){std::this_thread::sleep_for(std::chrono::milliseconds(3));worker.update(dirty,result,job);}check(result==2,"optional full-quality-only preview");
 psui::AdaptivePreview<int> responsive;
 auto quick=[](psui::PreviewPass pass,const auto&){return pass.draft?1:2;};
 dirty=true;responsive.update(dirty,result,quick);
 for(int i=0;i<150&&!responsive.full_frames;++i){
  std::this_thread::sleep_for(std::chrono::milliseconds(4));responsive.update(dirty,result,quick);
 }
 check(responsive.full_frames==1,"native timing warmup");
 const auto draft_count=responsive.draft_frames;
 int precise=0;
 for(int i=0;i<20;++i){
  dirty=true;if(responsive.update(dirty,result,quick)&&result==2)++precise;
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
 }
 check(precise>=5&&responsive.draft_frames==draft_count,"in-budget drag publishes native frames");
 std::atomic<bool> expensive_started=false;
 auto sudden_cost=[&](psui::PreviewPass pass,const auto& cancel){
  if(pass.draft)return 1;
  expensive_started=true;
  const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(500);
  while(std::chrono::steady_clock::now()<end){
   if(cancel())return -10;
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 2;
 };
 dirty=true;responsive.update(dirty,result,sudden_cost);
 for(int i=0;i<80&&!expensive_started;++i){
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
 }
 check(expensive_started,"native job was started after a parameter change");
 bool fell_back=false;
 for(int i=0;i<45&&!fell_back;++i){
  dirty=true;fell_back=responsive.update(dirty,result,sudden_cost)&&result==1;
  std::this_thread::sleep_for(std::chrono::milliseconds(6));
 }
 check(fell_back,"superseded heavy native job yields to drafts during interaction");
 responsive.stop();
 // Starglow: reach and brightness must be independent, rotations stay thin,
 // and broad anime-style highlights must not blow out every direction.
 using namespace lightfx;Image impulse{257,257,0,0,1,std::vector<float>(257*257*4)};const int cx=128,cy=128;auto at=[&](int x,int y){return (std::size_t(y)*257+x)*4;};auto center=at(cx,cy);for(int k=0;k<4;k++)impulse.rgba[center+k]=1;
 auto p=defaults(Kind::starglow,257,257);p.threshold=.75;p.softness=0;p.lengths.fill(0);p.lengths[0]=100;p.length=40;Image out;
 check(render(impulse,p,out),"star threshold highlight render");auto energy=[&](int x,int y){auto i=at(x,y);return double(out.rgba[i])*out.rgba[i+3];};
 const double short_peak=energy(129,128);check(short_peak>.3&&energy(128,128)==1,"visible ray without repeated source energy");p.length=160;check(render(impulse,p,out),"long streak");check(energy(129,128)>=short_peak&&energy(129,128)<short_peak*1.12,"length does not dim near highlight");
 for(int angle:{13,29,77,137,211,319}){p.rotation=angle;check(render(impulse,p,out),"arbitrary star rotation");double total=0,wide=0;const double a=angle*3.141592653589793/180,dx=std::cos(a),dy=std::sin(a);for(int y=0;y<257;y++)for(int x=0;x<257;x++){const double along=(x-cx)*dx+(y-cy)*dy,across=std::abs(-(x-cx)*dy+(y-cy)*dx);if(along>20&&along<70){double e=energy(x,y);total+=e;if(across>2.2)wide+=e;}}check(total>1&&wide/total<.005,"rotated streak stays within narrow line footprint");}
 Image panorama{16384,4,0,0,1,std::vector<float>(16384*4*4)};for(int k=0;k<4;k++)panorama.rgba[(2*16384+8192)*4+k]=1;p.rotation=13;auto begin=std::chrono::steady_clock::now();check(render(panorama,p,out),"thin panoramic image");check(std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<5,"sheared scan clips empty space on panoramic images");
 auto panel=impulse;for(int y=90;y<167;y++)for(int x=90;x<167;x++){auto i=at(x,y);for(int k=0;k<4;k++)panel.rgba[i+k]=1;}p.rotation=0;p.lengths.fill(100);p.source_opacity=0;check(render(panel,p,out),"broad flat highlight");for(std::size_t i=0;i<out.rgba.size();i+=4)check(double(out.rgba[i])*out.rgba[i+3]<=1.00001,"generated glow bounded at unit boost");
 if(argc>1){std::filesystem::path dir=argv[1];std::filesystem::create_directories(dir);p=defaults(Kind::starglow,257,257);p.length=95;p.threshold=.8;p.lengths={100,25,100,25,100,25,100,25};Image dots=impulse;for(auto [x,y]:{std::pair{45,52},std::pair{201,65},std::pair{65,196},std::pair{197,188}}){auto i=at(x,y);for(int k=0;k<4;k++)dots.rgba[i+k]=1;}render(dots,p,out);std::ofstream f(dir/"starglow-lines.ppm",std::ios::binary);f<<"P6\n257 257\n255\n";for(std::size_t i=0;i<out.rgba.size();i+=4)for(int k=0;k<3;k++){auto v=std::uint8_t(std::clamp(out.rgba[i+k]*out.rgba[i+3],0.f,1.f)*255);f.write(reinterpret_cast<char*>(&v),1);}}
 std::cout<<"PASS: "<<checks<<" assertions; native/full snapshots; alpha/energy/coordinates; valid drafts; continuous drag frames="<<seen<<"; max submit ms="<<submit_ms<<"; automatic refinement; star intensity/reach/rotation/flat highlights.\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
