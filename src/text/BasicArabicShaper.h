#pragma once

#include "TextTypes.h"

namespace textsys
{

/*
 * Shaper عربي مبسط:
 * - يشكل الأحرف العربية الشائعة إلى Arabic Presentation Forms
 * - يدعم isolated / initial / medial / final
 * - يدعم lam-alef ligature الأكثر شيوعاً
 *
 * الحدود الحالية:
 * - لا يطبق mark reordering الكامل
 * - لا يطبق GSUB/GPOS
 * - لا يغطي كل لغات السكربت العربي خارج جدول الحروف الشائعة
 */
class BasicArabicShaper final : public ITextShaper
{
  public:
    [[nodiscard]] std::vector<GlyphInfo> shapeRun(const TextRun& run) const override;
};

} // namespace textsys
