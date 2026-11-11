#pragma once
#include <string>
#include <string_view>
namespace psui {
// Modern PS returns propLocale as UTF-16LE; older hosts may use byte strings.
// Locale codes contain ASCII only. Read bytes without alignment assumptions.
inline std::string decode_host_locale(std::string_view bytes) {
  auto byte = [&](std::size_t i) {
    return static_cast<unsigned char>(bytes[i]);
  };
  const bool le_bom = bytes.size() >= 2 && byte(0) == 0xff && byte(1) == 0xfe;
  const bool be_bom = bytes.size() >= 2 && byte(0) == 0xfe && byte(1) == 0xff;
  const bool le = le_bom || (bytes.size() >= 2 && byte(0) != 0 && byte(1) == 0);
  const bool be = be_bom || (bytes.size() >= 2 && byte(0) == 0 && byte(1) != 0);
  std::string locale;
  auto valid = [](unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' ||
           c == '@';
  };
  if (le || be) {
    if (bytes.size() % 2)
      return {};
    for (std::size_t i = le_bom || be_bom ? 2 : 0; i < bytes.size(); i += 2) {
      const auto low = byte(i + (le ? 0 : 1)), high = byte(i + (le ? 1 : 0));
      if (!low && !high)
        break;
      if (high || !valid(low))
        return {};
      locale += static_cast<char>(low);
    }
  } else {
    const std::size_t start = bytes.size() >= 3 && byte(0) == 0xef &&
                                      byte(1) == 0xbb && byte(2) == 0xbf
                                  ? 3
                                  : 0;
    for (std::size_t i = start; i < bytes.size() && byte(i); ++i) {
      if (!valid(byte(i)))
        return {};
      locale += bytes[i];
    }
  }
  return locale;
}
enum class Language { Chinese, English };
enum class LanguageMode { follow_host, Chinese, English };
// Photoshop locale, not the Windows display language. Unknown locales use
// English.
inline Language language_for_locale(std::string_view locale) {
  return locale.size() >= 2 && (locale[0] == 'z' || locale[0] == 'Z') &&
                 (locale[1] == 'h' || locale[1] == 'H') &&
                 (locale.size() == 2 || locale[2] == '_' || locale[2] == '-')
             ? Language::Chinese
             : Language::English;
}
inline Language resolve_language(LanguageMode mode, std::string_view locale) {
  return mode == LanguageMode::follow_host ? language_for_locale(locale)
         : mode == LanguageMode::Chinese   ? Language::Chinese
                                           : Language::English;
}
// Effect metadata uses UTF-8 "中文 / English". This changes presentation only;
// the original metadata remains the stable identity of every control.
inline std::string_view localized_text(std::string_view text,
                                       Language language) {
  const auto split = text.find(" / ");
  if (split == std::string_view::npos)
    return text;
  return language == Language::Chinese ? text.substr(0, split)
                                       : text.substr(split + 3);
}
} // namespace psui
