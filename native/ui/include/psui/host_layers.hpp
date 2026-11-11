#pragma once
#include "photoshop_host.hpp"
#include <array>
#include <string>
#include <vector>
namespace psui {
struct HostLayer {
  unsigned32 id=0;
  std::uint64_t name_key=0;
  std::string name;
  std::array<ReadChannelDesc*,3> rgb{};
  ReadChannelDesc* alpha=nullptr;
  bool target=false;
};
inline std::uint64_t layer_name_key(const std::string& name){
  std::uint64_t key=14695981039346656037ULL;
  for(unsigned char c:name)key=(key^c)*1099511628211ULL;
  return key;
}
// Descriptors/ports are borrowed only for the current filter invocation.
std::vector<HostLayer> host_layers(FilterRecord*);
struct LayerPixels {
  VRect bounds{};
  int width=0,height=0,depth=8;
  std::vector<float> rgba;
};
// Read-only snapshot; never switches layers, changes visibility or writes the document.
// max_side=0 preserves native resolution; otherwise reads a bounded proxy.
OSErr host_read_layer(FilterRecord*,const HostLayer&,VRect bounds,int max_side,LayerPixels&);
}
