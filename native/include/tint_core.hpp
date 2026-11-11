#pragma once
#include <cstddef>
#include <cstdint>

namespace tint {
// Independent reconstruction model; not yet calibrated against Adobe Tint.
// Colors use the numeric encoding of the input buffer. No ICC conversion here.
struct Parameters {
    double black[3] = {0.0, 0.0, 0.0};
    double white[3] = {1.0, 1.0, 1.0};
    double amount = 1.0; // normalized 0..1
};
enum class SampleType { u8, photoshop_u16, f32 };
struct ConstImage {
    const void* data = nullptr;
    std::size_t capacity = 0;
    std::size_t width = 0, height = 0, row_bytes = 0;
    unsigned components = 4; // packed RGB or RGBA; alpha is straight/unassociated
    SampleType type = SampleType::u8;
};
struct Image {
    void* data = nullptr;
    std::size_t capacity = 0;
    std::size_t width = 0, height = 0, row_bytes = 0;
    unsigned components = 4;
    SampleType type = SampleType::u8;
};
enum class Status { ok, invalid_parameters, invalid_layout, overlapping_buffers };
// Supports disjoint buffers or exact in-place operation, positive row strides,
// and unaligned rows. Padding is untouched. No allocations, threads or host API.
Status process(const ConstImage&, const Image&, const Parameters&) noexcept;
const char* status_text(Status) noexcept;
} // namespace tint
