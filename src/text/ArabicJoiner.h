#pragma once

#include "TextTypes.h"

#include <cstdint>

namespace textsys::arabic
{

struct ArabicShapeEntry
{
    char32_t baseCodepoint = U'\0';
    JoiningType joiningType = JoiningType::NonJoining;
    char32_t isolated = U'\0';
    char32_t final = U'\0';
    char32_t initial = U'\0';
    char32_t medial = U'\0';
};

/*
 * يعيد بيانات الحرف العربي إن كان مدعوماً داخل الـ shaper الحالي.
 */
[[nodiscard]] const ArabicShapeEntry* findShapeEntry(char32_t codepoint);

/*
 * يحدد إن كان المحرف حرفاً عربياً يدخل في قرارات الوصل.
 */
[[nodiscard]] bool isArabicLetter(char32_t codepoint);

/*
 * بعض العلامات العربية لا تُشكّل وحدها لكنها لا يجب أن تقطع الوصل.
 */
[[nodiscard]] bool isTransparentMark(char32_t codepoint);

/*
 * يعطي نوع الوصل المستخدم في الشروط المنطقية.
 */
[[nodiscard]] JoiningType getJoiningType(char32_t codepoint);

/*
 * هل يستطيع الحرف الاتصال مع محرف قبله منطقياً؟
 * هذا يعني وجود طرف اتصال على الجهة اليمنى بصرياً.
 */
[[nodiscard]] bool canConnectRight(char32_t codepoint);

/*
 * هل يستطيع الحرف الاتصال مع محرف بعده منطقياً؟
 * هذا يعني وجود طرف اتصال على الجهة اليسرى بصرياً.
 */
[[nodiscard]] bool canConnectLeft(char32_t codepoint);

/*
 * يحول الحرف العربي الأساسي إلى شكل presentation form مناسب.
 * إن لم يتوفر شكل مناسب يرجع الحرف الأصلي.
 */
[[nodiscard]] char32_t getPresentationForm(char32_t codepoint, GlyphForm form);

/*
 * دعم انتقالي شائع لرباط lam-alef.
 * نرجع true عند النجاح ونكتب glyph الناتج داخل outGlyph.
 */
[[nodiscard]] bool tryShapeLamAlef(char32_t lamCodepoint, char32_t alefCodepoint, bool connectsToPrevious,
                                   char32_t* outGlyph);

} // namespace textsys::arabic
