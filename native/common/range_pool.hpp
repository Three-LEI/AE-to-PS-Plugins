#pragma once
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>

namespace pscpu {
namespace detail {
inline thread_local bool range_worker=false;
}
// Owned by one render or one preview worker. Never keep live worker threads in
// a DLL static: Photoshop may unload the module after a filter call completes.
class RangePool {
 using Invoke=void(*)(void*,unsigned,bool);
 std::mutex mutex_;
 std::condition_variable ready_,finished_;
 std::array<std::thread,7> workers_;
 std::size_t generation_=0;
 unsigned count_=0,remaining_=0;
 bool stopping_=false,busy_=false;
 void* context_=nullptr;
 Invoke invoke_=nullptr;
 void worker(unsigned index,std::size_t seen){
  detail::range_worker=true;
  std::unique_lock lock(mutex_);
  for(;;){
   ready_.wait(lock,[&]{return stopping_||seen!=generation_;});
   if(stopping_)return;
   seen=generation_;
   if(index>=count_)continue;
   auto* context=context_;const auto invoke=invoke_;
   lock.unlock();invoke(context,index,false);lock.lock();
   if(!--remaining_)finished_.notify_one();
  }
 }
public:
 RangePool()=default;
 RangePool(const RangePool&)=delete;
 RangePool& operator=(const RangePool&)=delete;
 ~RangePool(){
  {std::lock_guard lock(mutex_);stopping_=true;}
  ready_.notify_all();
  for(auto& thread:workers_)if(thread.joinable())thread.join();
 }
 bool available()const{return !busy_&&!detail::range_worker;}
 unsigned worker_count()const{unsigned result=0;for(const auto& thread:workers_)result+=thread.joinable();return result;}
 template<class Run,class Poll> void execute(unsigned count,Run& run,Poll poll){
  // The owner alone submits. Run and Poll must contain their own exception
  // handling so every launched range completes before this stack is released.
  std::array<bool,8> launched{};
  {
   std::lock_guard lock(mutex_);busy_=true;remaining_=0;
   for(unsigned i=1;i<count;i++){
    auto& thread=workers_[i-1];
    if(!thread.joinable())try{thread=std::thread([this,i,seen=generation_]{worker(i,seen);});}catch(...){/* Caller owns this range. */}
    if(thread.joinable()){launched[i]=true;++remaining_;}
   }
   count_=count;context_=&run;invoke_=[](void* context,unsigned index,bool caller){(*static_cast<Run*>(context))(index,caller);};++generation_;
  }
  ready_.notify_all();
  run(0,true);for(unsigned i=1;i<count;i++)if(!launched[i])run(i,true);
  std::unique_lock lock(mutex_);
  while(remaining_){
   // Completion wakes immediately; timeout only polls the host's cancel hook.
   if(finished_.wait_for(lock,std::chrono::milliseconds(1),[&]{return remaining_==0;}))break;
   lock.unlock();poll();lock.lock();
  }
  context_=nullptr;invoke_=nullptr;busy_=false;
 }
};
namespace detail {
inline thread_local RangePool* active_pool=nullptr;
}
// Nested calls inherit the caller's pool. No background thread survives the
// outer scope, including cancellation, exceptions and DLL adapter tests.
class ParallelScope {
 RangePool local_;
 RangePool* previous_=detail::active_pool;
public:
 ParallelScope(){if(!previous_)detail::active_pool=&local_;}
 explicit ParallelScope(RangePool& pool){detail::active_pool=&pool;}
 ParallelScope(const ParallelScope&)=delete;
 ~ParallelScope(){detail::active_pool=previous_;}
};
}
