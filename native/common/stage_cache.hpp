#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
namespace pscpu {
// Single render-thread owner; inputs in the key must be immutable snapshots.
// Failed/cancelled builders never publish a partial value. Byte cost includes
// all retained buffers and is supplied by the effect, not estimated here.
template<class Key,class Value,std::size_t Slots=2> class StageCache {
 static_assert(Slots>0);
 struct Entry {std::optional<Key> key;Value value;std::size_t bytes=0;std::uint64_t used=0;};
 std::array<Entry,Slots> entries_;
 std::size_t budget_,bytes_=0,hits_=0,builds_=0;
 std::uint64_t clock_=0;
 void discard(Entry& entry){bytes_-=entry.bytes;entry=Entry{};}
public:
 explicit StageCache(std::size_t budget=96u*1024*1024):budget_(budget){}
 bool fits(std::size_t bytes)const{return bytes<=budget_;}
 void clear(){for(auto& entry:entries_)discard(entry);}
 std::size_t bytes()const{return bytes_;}
 std::size_t hits()const{return hits_;}
 std::size_t builds()const{return builds_;}
 template<class Build>const Value* get(const Key& key,std::size_t bytes,Build build){
  if(!fits(bytes))return nullptr;
  for(auto& entry:entries_)if(entry.key&&*entry.key==key){entry.used=++clock_;++hits_;return &entry.value;}
  auto* target=&entries_[0];
  for(auto& entry:entries_)if(entry.used<target->used)target=&entry;
  discard(*target);
  while(bytes_>budget_-bytes){
   Entry* oldest=nullptr;for(auto& entry:entries_)if(entry.key&&(!oldest||entry.used<oldest->used))oldest=&entry;
   if(!oldest)break;discard(*oldest);
  }
  ++builds_;Value value;
  if(!build(value))return nullptr;
  target->value=std::move(value);target->key=key;target->bytes=bytes;target->used=++clock_;bytes_+=bytes;
  return &target->value;
 }
};
}
