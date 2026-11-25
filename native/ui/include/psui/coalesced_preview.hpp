#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include "../../../common/range_pool.hpp"
namespace psui {
enum class PreviewRequest { Draft, InteractiveFull, Final };

// One running request and one latest queued request. Interactive work can
// finish across slider events, while final work is tied to its exact revision.
template<class Result>class CoalescedPreview {
 using Cancel=std::function<bool()>;
 using Clock=std::chrono::steady_clock;
 struct Job {
  std::uint64_t revision;
  PreviewRequest mode;
  std::chrono::milliseconds budget;
  std::function<Result(const Cancel&)> work;
 };
 struct Done{std::uint64_t revision;PreviewRequest mode;Result value;std::exception_ptr error;};
 std::mutex mutex;std::condition_variable condition;std::optional<Job> queued;
 std::optional<Done> completed;std::atomic<std::uint64_t> latest=0,minimum=0;
 std::atomic<bool> slow_interaction=false;
 bool running=false;std::jthread worker;
 void run(std::stop_token token){
  pscpu::ParallelScope parallel;
  while(!token.stop_requested()){
   std::optional<Job> job;{
    std::unique_lock lock(mutex);condition.wait(lock,[&]{return token.stop_requested()||queued.has_value();});
    if(token.stop_requested())return;job=std::move(queued);queued.reset();running=true;
   }
   Done done{job->revision,job->mode,{},nullptr};
   const auto deadline=Clock::now()+job->budget;
   const Cancel cancel=[&] {
    if(token.stop_requested()||job->revision<minimum.load())return true;
    if(job->revision==latest.load())return false;
    if(job->mode==PreviewRequest::Final)return true;
    if(job->budget.count()>0&&Clock::now()>=deadline){
     slow_interaction.store(true);
     return true;
    }
    return false;
   };
   try{done.value=job->work(cancel);}catch(...){done.error=std::current_exception();}
   {std::lock_guard lock(mutex);running=false;if(!cancel()&&job->revision>=minimum.load())completed=std::move(done);}
  }
 }
public:
 CoalescedPreview():worker([this](std::stop_token token){run(token);}){}
 CoalescedPreview(const CoalescedPreview&)=delete;
 ~CoalescedPreview(){worker.request_stop();condition.notify_all();if(worker.joinable())worker.join();}
 template<class Work>std::uint64_t submit(PreviewRequest mode,Work work,std::chrono::milliseconds budget={}){
  const auto revision=latest.fetch_add(1)+1;
  {std::lock_guard lock(mutex);queued=Job{revision,mode,budget,std::move(work)};}
  condition.notify_one();
  return revision;
 }
 template<class Work>std::uint64_t submit(bool draft,Work work){
  return submit(draft?PreviewRequest::Draft:PreviewRequest::Final,std::move(work));
 }
 void invalidate(){
  std::lock_guard lock(mutex);
  minimum=latest.fetch_add(1)+1;
  queued.reset();
  completed.reset();
  slow_interaction.store(false);
 }
 bool take_slow_interaction(){return slow_interaction.exchange(false);}
 bool take(Result& value,bool& draft,std::uint64_t& revision){
  std::optional<Done> done;{std::lock_guard lock(mutex);if(!completed)return false;done=std::move(completed);completed.reset();}
  if(done->revision<minimum.load()||(done->mode==PreviewRequest::Final&&done->revision!=latest.load()))return false;
  if(done->error)std::rethrow_exception(done->error);
  value=std::move(done->value);draft=done->mode==PreviewRequest::Draft;revision=done->revision;return true;
 }
 bool pending(){std::lock_guard lock(mutex);return running||queued.has_value();}
};
} // namespace psui
