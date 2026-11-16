#pragma once
#include <cstddef>
#include <vector>
namespace pscpu {
// Reuse capacity between passes. Callers overwrite every element before read;
// growing still value-initializes safely. Lifetime belongs to a render, so no
// image-sized allocation remains in a DLL global after Photoshop unloads it.
template<class T>class ScratchBuffer {
 std::vector<T> buffer_;
public:
 std::vector<T>& resize_for_overwrite(std::size_t size){buffer_.resize(size);return buffer_;}
};
}
