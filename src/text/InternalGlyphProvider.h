#pragma once

#include "TextTypes.h"

namespace textsys
{

/*
 * مزود قياسات داخلي مؤقت يسمح بتطوير الـ layout والـ renderer
 * من دون FreeType أو أطلس glyphs فعلي.
 *
 * الهدف منه أن يبقى بديلاً نظيفاً يمكن استبداله لاحقاً مباشرةً.
 */
class InternalGlyphProvider final : public IGlyphMetricsProvider
{
  public:
    explicit InternalGlyphProvider(float emSize = 1.0f);

    void setEmSize(float emSize);
    [[nodiscard]] float emSize() const noexcept;
    [[nodiscard]] GlyphMetrics queryGlyphMetrics(const GlyphInfo& glyph) const override;

  private:
    float mEmSize = 1.0f;
};

} // namespace textsys
