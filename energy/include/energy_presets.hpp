#pragma once
namespace energy {
// The menu's first entry is Custom; the remaining entries map to preset 0..N.
inline constexpr const char* unified_preset_labels[]={
 "自定义 / Custom","默认光束 / Neon","电流 / Electric","燃烧 / Burning","混合 / Mixed",
 "薄荷细光 / Mint Thread","玫瑰霓虹 / Rose Neon","金色电弧 / Golden Arc","冰霜 / Frost",
 "紫雾 / Violet Mist","幽绿 / Venom","熔岩 / Lava","日冕 / Solar Corona",
 "等离子 / Plasma","流风 / Windborne","幽光 / Ghost Light","极光 / Aurora"
};
inline constexpr int unified_preset_count=sizeof(unified_preset_labels)/sizeof(*unified_preset_labels)-1;
}
