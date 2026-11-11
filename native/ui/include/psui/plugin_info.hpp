#pragma once
#include "localization.hpp"
#include "source_repository.hpp"
#include <string>
namespace psui {
struct PluginInfo {
  const char *name, *version;
};
inline constexpr const char *about_disclaimer =
    "本插件为独立复刻，与 AE 原版效果并非百分之百相同。 / "
    "This is an independent reconstruction. Its results are not 100% identical "
    "to the original After Effects effect.";
inline constexpr const char *about_author = "by：淚泪泪";
inline constexpr const char *about_source = "开源地址 / Source code";
inline std::string about_text(const PluginInfo &info, Language language, bool include_source = true) {
  return std::string(info.name) + "\n" +
         (language == Language::Chinese ? "版本：" : "Version: ") +
         info.version + "\n\n" +
         std::string(localized_text(about_disclaimer, language)) + "\n\n" +
         about_author +
         (include_source ? "\n" + std::string(localized_text(about_source, language)) +
                               "\n" + source_repository : "");
}
// Legacy Photoshop Help > About Plug-in uses the same metadata and statement.
void show_host_about(const PluginInfo &, const wchar_t *title);
} // namespace psui
