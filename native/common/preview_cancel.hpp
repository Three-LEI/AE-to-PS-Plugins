#pragma once
#include <functional>
namespace pscpu {
// Opt-in tag for small preview-only workloads where launching a group of
// threads costs more than the arithmetic. Native filter jobs never carry it.
struct SerialPreviewCancel {
 std::function<bool()> callback;
 bool operator()()const{return callback&&callback();}
};
}
