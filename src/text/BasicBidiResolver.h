#pragma once

#include "TextTypes.h"

namespace textsys
{

/*
 * محلل BiDi مبسط وموجه لواجهات الألعاب وHUD:
 * - يدعم RTL العربي وLTR اللاتيني والأرقام
 * - يحافظ على الأرقام كسلاسل LTR
 * - يعيد ترتيب الـ runs بصرياً من دون تطبيق UAX#9 الكامل
 */
class BasicBidiResolver final : public IBidiResolver
{
  public:
    [[nodiscard]] std::vector<TextRun> resolveVisualRuns(const UnicodeText& text) const override;

  private:
    [[nodiscard]] TextDirection resolveBaseDirection(const UnicodeText& text) const;
};

} // namespace textsys
