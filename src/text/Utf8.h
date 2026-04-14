#pragma once

#include <string>
#include <string_view>

namespace textsys::utf8
{

inline constexpr char32_t kReplacementCharacter = U'\uFFFD';

/*
 * يفك UTF-8 إلى UTF-32 مع validation آمن.
 * عند أي تسلسل معطوب نكتب U+FFFD ونكمل القراءة.
 */
[[nodiscard]] std::u32string decode(std::string_view utf8, bool* hadErrors = nullptr);
[[nodiscard]] std::u32string decode(std::u8string_view utf8, bool* hadErrors = nullptr);

/*
 * يعيد ترميز UTF-32 إلى UTF-8.
 * أي codepoint غير صالح يستبدل بـ U+FFFD.
 */
[[nodiscard]] std::string encode(std::u32string_view codepoints, bool* hadErrors = nullptr);

/*
 * فحص سريع لصحة السلسلة على مستوى UTF-8.
 */
[[nodiscard]] bool isValid(std::string_view utf8);
[[nodiscard]] bool isValid(std::u8string_view utf8);

} // namespace textsys::utf8
