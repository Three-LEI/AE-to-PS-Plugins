#include "tint_core.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

namespace tint {
namespace {
bool multiply(std::size_t a, std::size_t b, std::size_t& out) noexcept {
    if (b && a > std::numeric_limits<std::size_t>::max() / b) return false;
    out = a * b;
    return true;
}
bool extent(std::size_t w, std::size_t h, std::size_t row,
            unsigned channels, std::size_t bytes, std::size_t cap,
            std::size_t& span) noexcept {
    std::size_t packed, offset;
    if (!multiply(w, channels * bytes, packed) || row < packed ||
        !multiply(h - 1, row, offset) || offset > cap || packed > cap - offset) return false;
    span = offset + packed;
    return true;
}
template<class T> T load(const unsigned char* ptr) noexcept {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    return value;
}
template<class T> void store(unsigned char* ptr, T value) noexcept {
    std::memcpy(ptr, &value, sizeof(T));
}
template<class T> T quantize(double v, double maximum) noexcept {
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(v); // HDR/negative values remain unclipped.
    } else {
        if (v <= 0.0) return 0;
        if (v >= maximum) return static_cast<T>(maximum);
        return static_cast<T>(v + 0.5);
    }
}
template<class T, unsigned Channels>
void render(const ConstImage& in, const Image& out, const Parameters& p, double maximum) noexcept {
    double black[3], range[3];
    for (unsigned c = 0; c < 3; ++c) {
        black[c] = p.black[c] * maximum;
        range[c] = (p.white[c] - p.black[c]) * maximum;
    }
    for (std::size_t y = 0; y < in.height; ++y) {
        const auto* src = static_cast<const unsigned char*>(in.data) + y * in.row_bytes;
        auto* dst = static_cast<unsigned char*>(out.data) + y * out.row_bytes;
        if (p.amount == 0.0) {
            if (src != dst) std::memcpy(dst, src, in.width * Channels * sizeof(T));
            continue;
        }
        for (std::size_t x = 0; x < in.width; ++x, src += Channels*sizeof(T), dst += Channels*sizeof(T)) {
            // Read all RGB values before writing, allowing exact in-place use.
            const double rgb[3] = {static_cast<double>(load<T>(src)),
                                   static_cast<double>(load<T>(src+sizeof(T))),
                                   static_cast<double>(load<T>(src+2*sizeof(T)))};
            const double y_value = (0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2]) / maximum;
            for (unsigned c = 0; c < 3; ++c) {
                const double mapped = black[c] + range[c]*y_value;
                store<T>(dst+c*sizeof(T), quantize<T>(rgb[c]+p.amount*(mapped-rgb[c]), maximum));
            }
            if constexpr (Channels == 4) {
                // Copy alpha bits without numeric conversion, including float NaNs.
                if (dst != src) std::memcpy(dst+3*sizeof(T), src+3*sizeof(T), sizeof(T));
            }
        }
    }
}
template<class T> void dispatch(const ConstImage& in, const Image& out, const Parameters& p, double max) noexcept {
    if (in.components == 3) render<T,3>(in,out,p,max);
    else render<T,4>(in,out,p,max);
}
} // namespace

Status process(const ConstImage& in, const Image& out, const Parameters& p) noexcept {
    if (!std::isfinite(p.amount) || p.amount < 0.0 || p.amount > 1.0) return Status::invalid_parameters;
    for (unsigned c = 0; c < 3; ++c) {
        if (!std::isfinite(p.black[c]) || !std::isfinite(p.white[c]) ||
            p.black[c]<0 || p.black[c]>1 || p.white[c]<0 || p.white[c]>1) return Status::invalid_parameters;
    }
    if (in.type != out.type || in.width != out.width || in.height != out.height ||
        in.components != out.components || (in.components != 3 && in.components != 4)) return Status::invalid_layout;
    std::size_t bytes;
    switch(in.type) {
        case SampleType::u8: bytes=1; break;
        case SampleType::photoshop_u16: bytes=2; break;
        case SampleType::f32: bytes=4; break;
        default: return Status::invalid_layout;
    }
    if (in.width == 0 || in.height == 0) return Status::ok;
    std::size_t src_span, dst_span;
    if (!in.data || !out.data || !extent(in.width,in.height,in.row_bytes,in.components,bytes,in.capacity,src_span) ||
        !extent(out.width,out.height,out.row_bytes,out.components,bytes,out.capacity,dst_span)) return Status::invalid_layout;
    const auto begin_in = reinterpret_cast<std::uintptr_t>(in.data);
    const auto begin_out = reinterpret_cast<std::uintptr_t>(out.data);
    const auto limit = std::numeric_limits<std::uintptr_t>::max();
    if(src_span > limit-begin_in || dst_span > limit-begin_out) return Status::invalid_layout;
    if (in.data == out.data) {
        if (in.row_bytes != out.row_bytes) return Status::overlapping_buffers;
    } else if (begin_in < begin_out+dst_span && begin_out < begin_in+src_span) return Status::overlapping_buffers;
    switch(in.type) {
        case SampleType::u8: dispatch<std::uint8_t>(in,out,p,255.0); break;
        case SampleType::photoshop_u16: dispatch<std::uint16_t>(in,out,p,32768.0); break;
        case SampleType::f32: dispatch<float>(in,out,p,1.0); break;
    }
    return Status::ok;
}
const char* status_text(Status s) noexcept {
    switch(s) {
        case Status::ok: return "ok";
        case Status::invalid_parameters: return "invalid parameters";
        case Status::invalid_layout: return "invalid image layout";
        case Status::overlapping_buffers: return "unsupported partial buffer overlap";
    }
    return "unknown status";
}
} // namespace tint
