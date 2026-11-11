#pragma once
#include <atomic>
#include <exception>
#include <thread>
#include <utility>
namespace psui {
// Only CPU work belongs here. Host callbacks, ImGui and texture uploads stay on
// the UI thread. Replacing a job cancels and joins it before its inputs can die.
template<class Result> class AsyncPreview {
 std::jthread worker;
 std::atomic<bool> ready=false;
 Result result;
 std::exception_ptr error;
public:
 AsyncPreview()=default;
 AsyncPreview(const AsyncPreview&)=delete;
 ~AsyncPreview(){stop();}
 void stop(){if(worker.joinable()){worker.request_stop();worker.join();}ready=false;}
 template<class Work> void start(Work work){
  stop();error=nullptr;result=Result{};
  worker=std::jthread([this,work=std::move(work)](std::stop_token token) mutable {
   try{result=work(token);}catch(...){error=std::current_exception();}
   ready.store(true,std::memory_order_release);
  });
 }
 bool pending() const{return worker.joinable()&&!ready.load(std::memory_order_acquire);}
 bool take(Result& next){
  if(!ready.load(std::memory_order_acquire))return false;
  worker.join();ready=false;if(error)std::rethrow_exception(error);
  next=std::move(result);return true;
 }
};
}
