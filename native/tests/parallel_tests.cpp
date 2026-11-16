#include "../common/parallel_ranges.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
 const auto caller=std::this_thread::get_id();std::vector<std::atomic<int>> hits(1003);
 const int old_round=std::fegetround();std::fesetround(FE_DOWNWARD);const auto mode=std::fegetround();
 const auto csr=_mm_getcsr();
 require(pscpu::parallel_ranges(hits.size(),2000000,[&]{require(std::this_thread::get_id()==caller,"host callback escaped caller");return false;},
 [&](std::size_t first,std::size_t last,const auto& stop){
  require(std::fegetround()==mode&&_mm_getcsr()==csr,"floating environment differs");
  for(auto i=first;i<last;++i){if(stop())return false;++hits[i];}return true;
 }),"parallel success");std::fesetround(old_round);
 for(auto& hit:hits)require(hit==1,"range duplicated or missing");
 require(!pscpu::parallel_ranges(100,2000000,[&]{require(std::this_thread::get_id()==caller,"cancel callback thread");return true;},
 [](auto,auto,const auto&){throw std::runtime_error("cancelled job ran");return true;}),"early cancel");
 bool caught=false;try{pscpu::parallel_ranges(100,2000000,{},[](auto,auto,const auto&)->bool{throw std::runtime_error("worker error");});}catch(const std::runtime_error&){caught=true;}
 require(caught,"worker exception lost");
 std::atomic<int> active=0;int calls=0;
 require(!pscpu::parallel_ranges(1000,2000000,[&]{require(std::this_thread::get_id()==caller,"mid-job callback thread");return ++calls>3;},
 [&](auto first,auto last,const auto& stop){++active;for(auto i=first;i<last;++i){if(stop()){--active;return false;}std::this_thread::sleep_for(std::chrono::microseconds(20));}--active;return true;}),"mid-job cancel");
 require(active==0,"workers escaped lifetime");
 {
  pscpu::RangePool pool;pscpu::ParallelScope scope(pool);
  std::array<std::thread::id,8> ids{};
  for(int iteration=0;iteration<40;iteration++){
   std::array<std::thread::id,8> now{};
   const int rounding=iteration%2?FE_DOWNWARD:FE_UPWARD;std::fesetround(rounding);const auto expected=_mm_getcsr();
   require(pscpu::parallel_ranges(8,2000000,{},[&](auto first,auto last,const auto& stop){
    require(std::fegetround()==rounding&&_mm_getcsr()==expected,"reused floating environment differs");
    for(auto i=first;i<last;i++)now[i]=std::this_thread::get_id();
    if(iteration==17)require(pscpu::parallel_ranges(3,2000000,stop,[](auto,auto,const auto&){return true;}),"nested work failed");
    std::fesetround(FE_TOWARDZERO);return true;
   }),"reused job failed");
   if(!iteration)ids=now;else require(now==ids,"workers were recreated between stages");
  }
  require(pool.worker_count()<=7,"unbounded worker count");
  bool thrown=false;try{pscpu::parallel_ranges(8,2000000,{},[](auto,auto,const auto&)->bool{throw std::runtime_error("pooled failure");});}catch(...){thrown=true;}
  require(thrown,"pooled exception lost");
  require(pscpu::parallel_ranges(8,2000000,{},[](auto,auto,const auto&){return true;}),"pool poisoned by failure");
  std::fesetround(old_round);
 }
 for(int i=0;i<24;i++){pscpu::ParallelScope scope;require(pscpu::parallel_ranges(32,2000000,{},[](auto,auto,const auto&){return true;}),"scope close failed");}
 std::atomic<int> owners=0;std::array<std::jthread,3> callers;
 for(auto& t:callers)t=std::jthread([&]{pscpu::ParallelScope scope;for(int i=0;i<10;i++)require(pscpu::parallel_ranges(16,2000000,{},[](auto,auto,const auto&){return true;}),"concurrent owner failed");++owners;});
 for(auto& t:callers)t.join();require(owners==3,"concurrent pools did not complete");
 std::cout<<"PASS: exact disjoint coverage, caller-only host cancellation, floating environment, exception propagation and joined lifetime.\n";
 std::cout<<"PASS: reused worker identities, per-job floating environment, nested work, recovery, scope shutdown and concurrent owners.\n";
}
