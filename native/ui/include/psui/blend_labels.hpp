#pragma once
#include "../../../common/pixel_blend.hpp"
namespace psui {
inline constexpr const char* blend_labels[]={
 "无（仅图案） / None (Pattern Only)","正常 / Normal","相加 / Add","正片叠底 / Multiply","滤色 / Screen","叠加 / Overlay","柔光 / Soft Light","强光 / Hard Light",
 "变暗 / Darken","变亮 / Lighten","差值 / Difference","排除 / Exclusion","减去 / Subtract","划分 / Divide","颜色加深 / Color Burn","颜色减淡 / Color Dodge","线性加深 / Linear Burn","线性光 / Linear Light","亮光 / Vivid Light","点光 / Pin Light","实色混合 / Hard Mix",
 "色相 / Hue","饱和度 / Saturation","颜色 / Color","明度 / Luminosity","在后 / Behind","Alpha 模板 / Stencil Alpha","亮度模板 / Stencil Luma","Alpha 轮廓 / Silhouette Alpha","亮度轮廓 / Silhouette Luma","溶解 / Dissolve"};
static_assert(sizeof(blend_labels)/sizeof(*blend_labels)==int(pscore::Blend::count));
}
