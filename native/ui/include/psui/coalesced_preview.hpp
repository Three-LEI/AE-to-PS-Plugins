#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include "../../../common/range_pool.hpp"
namespace psui {
// Opt-in scheduler: submitting never joins a render thread. Running drafts may
// finish so continuous dragging produces frames; obsolete full jobs cancel.
template<class Result>class CoalescedPreview {
 using Cancel=std::function<bool()>;
 struct Job{std::uint64_t revision;bool draft;std::function<Result(const Cancel&)> work;};
 struct Done{std::uint64_t revision;bool draft;Result value;std::exception_ptr error;};
 std::mutex mutex;std::condition_variable condition;std::optional<Job> queued;
 std::optional<Done> completed;std::atomic<std::uint64_t> latest=0,minimum=0;
 bool running=false;std::jthread worker;
 void run(std::stop_token token){
  pscpu::ParallelScope parallel;
  while(!token.stop_requested()){
   std::optional<Job> job;{
    std::unique_lock lock(mutex);condition.wait(lock,[&]{return token.stop_requested()||queued.has_value();});
    if(token.stop_requested())return;job=std::move(queued);queued.reset();running=true;
   }
   Done done{job->revision,job->draft,{},nullptr};
   const Cancel cancel=[&]{return token.stop_requested()||(!job->draft&&latest.load()!=job->revision);};
   try{done.value=job->work(cancel);}catch(...){done.error=std::current_exception();}
   {std::lock_guard lock(mutex);running=false;if(!cancel()&&job->revision>=minimum.load())completed=std::move(done);}
  }
 }
public:
 CoalescedPreview():worker([this](std::stop_token token){run(token);}){}
 CoalescedPreview(const CoalescedPreview&)=delete;
 ~CoalescedPreview(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
 template<class Work>std::uint64_t submit(bool draft,Work work){
  const auto revision=latest.fetch_add(1)+1;{std::lock_guard lock(mutex);queued=Job{revision,draft,std::move(work)};}condition.notify_one();return revision;
 }
 void invalidate(){minimum=latest.fetch_add(1)+1;std::lock_guard lock(mutex);queued.reset();completed.reset();}
 bool take(Result& value,bool& draft,std::uint64_t& revision){
  std::optional<Done> done;{std::lock_guard lock(mutex);if(!completed)return false;done=std::move(completed);completed.reset();}
  if(done->revision<minimum.load()||(!done->draft&&done->revision!=latest.load()))return false;if(done->error)std::rethrow_exception(done->error);
  value=std::move(done->value);draft=done->draft;revision=done->revision;return true;
 }
 bool pending(){std::lock_guard lock(mutex);return running||queued.has_value();}
};
} // namespace psui
