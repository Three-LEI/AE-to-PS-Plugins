#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <system_error>
#include <thread>
#if defined(__SSE__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace pscpu {
// Disjoint output ranges only. Each pixel/line keeps its original arithmetic
// order. Cancellation callbacks run exclusively on the invoking (host) thread.
// All workers finish before captured buffers can be reused or destroyed.
template<class Work>
bool parallel_ranges(std::size_t rows,std::size_t units,
                     const std::function<bool()>& cancel,Work&& work){
 const auto cancelled=[&]{return cancel&&cancel();};
#ifdef AEPS_FORCE_SERIAL
 (void)units;
 return !cancelled()&&work(0,rows,cancelled);
#else
 const unsigned available=std::max(1u,std::thread::hardware_concurrency());
 const unsigned count=unsigned(std::min<std::size_t>({rows,available>1?available-1:1,units<1048576?4u:8u}));
 if(units<65536||count<2)return !cancelled()&&work(0,rows,cancelled);
 if(cancelled())return false;
 std::atomic<bool> stop=false;
 std::atomic<unsigned> done=0;
 std::array<std::exception_ptr,8> errors{};
 std::array<bool,8> launched{};
 std::fenv_t environment{};const bool have_environment=std::fegetenv(&environment)==0;
#if defined(__SSE__) || defined(_M_X64)
 const auto mxcsr=_mm_getcsr();
#endif
 auto run=[&](unsigned index,bool caller){
  auto check=[&]{
   if(stop.load(std::memory_order_relaxed))return true;
   if(caller&&cancelled()){stop=true;return true;}
   return false;
  };
  try{
   if(!caller){
    if(have_environment)std::fesetenv(&environment);
#if defined(__SSE__) || defined(_M_X64)
    _mm_setcsr(mxcsr);
#endif
   }
   if(!check()&&!work(rows*index/count,rows*(index+1)/count,check))stop=true;
  }catch(...){errors[index]=std::current_exception();stop=true;}
 };
 // Declare worker owners after every captured object so exceptional unwinding
 // also joins threads before their referenced state can be destroyed.
 std::array<std::jthread,7> workers;
 unsigned running=0;
 for(unsigned index=1;index<count;++index){
  try{
   workers[index-1]=std::jthread([&,index]{run(index,false);done.fetch_add(1,std::memory_order_release);});
   launched[index]=true;++running;
  }catch(...){/* The caller handles this disjoint range, including allocation failure. */}
 }
 run(0,true);
 for(unsigned index=1;index<count;++index)if(!launched[index])run(index,true);
 while(done.load(std::memory_order_acquire)!=running){
  try{if(cancelled())stop=true;}catch(...){errors[0]=std::current_exception();stop=true;}
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 for(auto& worker:workers)if(worker.joinable())worker.join();
 for(auto& error:errors)if(error)std::rethrow_exception(error);
 return !stop.load();
#endif
}
}
