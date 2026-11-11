#pragma once
#include <cstdint>
namespace psui {
// Composite the original and generated pixels over the same checker, then
// apply Photoshop's selection coverage. OutFillMask generators create opacity;
// ordinary/protected-transparency filters keep the input alpha.
inline std::uint8_t preview_channel(std::uint8_t original,
                                    std::uint8_t processed, std::uint8_t alpha,
                                    std::uint8_t selection,
                                    std::uint8_t checker, bool enabled,
                                    bool fill_opacity) {
  const unsigned before = original * alpha + checker * (255U - alpha);
  if (!enabled)
    return static_cast<std::uint8_t>((before + 127U) / 255U);
  const unsigned after_alpha = fill_opacity ? 255U : alpha;
  const unsigned after =
      processed * after_alpha + checker * (255U - after_alpha);
  return static_cast<std::uint8_t>(
      (before * (255U - selection) + after * selection + 32512U) / 65025U);
}
} // namespace psui
