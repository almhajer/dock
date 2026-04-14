#pragma once

#include "TextTypes.h"

#include <string_view>

namespace textsys
{

/*
 * خيارات تخطيط سطر واحد.
 */
struct TextLayoutOptions
{
    float originX = 0.0f;
    float baselineY = 0.0f;
    float fontScale = 1.0f;
    float lineHeight = 1.2f;
    float letterSpacing = 0.0f;
    TextAlignment alignment = TextAlignment::Start;
};

/*
 * orchestrator بين decoding -> bidi -> shaping -> layout.
 */
class TextLayouter
{
  public:
    TextLayouter(const IBidiResolver& bidiResolver, const ITextShaper& textShaper,
                 const IGlyphMetricsProvider& glyphMetricsProvider);

    [[nodiscard]] TextLayout layoutUtf8(std::string_view utf8Text, const TextLayoutOptions& options = {}) const;
    [[nodiscard]] TextLayout layoutUtf8(std::u8string_view utf8Text, const TextLayoutOptions& options = {}) const;
    [[nodiscard]] TextLayout layoutUnicode(const UnicodeText& text, const TextLayoutOptions& options = {}) const;

  private:
    const IBidiResolver& mBidiResolver;
    const ITextShaper& mTextShaper;
    const IGlyphMetricsProvider& mGlyphMetricsProvider;
};

} // namespace textsys
