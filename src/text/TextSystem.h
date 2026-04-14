#pragma once

#include "BasicArabicShaper.h"
#include "BasicBidiResolver.h"
#include "InternalGlyphProvider.h"
#include "TextLayouter.h"
#include "TextRendererBridge.h"

#include <string_view>

namespace textsys
{

/*
 * ناتج موحد جاهز للربط مع renderer:
 * layout منطقي + quads + vertices flat.
 */
struct PreparedText
{
    TextLayout layout;
    std::vector<GlyphQuad> quads;
    std::vector<TextVertex> vertices;
};

/*
 * خيارات البناء end-to-end من نص UTF-8 إلى vertices قابلة للرفع لـ Vulkan.
 */
struct TextSystemBuildOptions
{
    TextLayoutOptions layout = {};
    TextRenderStyle render = {};
};

/*
 * واجهة استخدام عالية المستوى فوق طبقات الـ text subsystem.
 * الهدف منها تقليل wiring اليدوي داخل HUD وواجهات اللعبة.
 */
class TextSystem final
{
  public:
    explicit TextSystem(float glyphEmSize = 1.0f);

    void setGlyphEmSize(float emSize);
    [[nodiscard]] float glyphEmSize() const noexcept;

    [[nodiscard]] TextLayout layoutUtf8(std::string_view utf8Text, const TextLayoutOptions& options = {}) const;
    [[nodiscard]] TextLayout layoutUtf8(std::u8string_view utf8Text, const TextLayoutOptions& options = {}) const;
    [[nodiscard]] TextLayout layoutUnicode(const UnicodeText& text, const TextLayoutOptions& options = {}) const;
    [[nodiscard]] std::vector<GlyphQuad> buildGlyphQuads(const TextLayout& layout,
                                                         const TextRenderStyle& style = {}) const;
    [[nodiscard]] std::vector<TextVertex> buildVertices(const TextLayout& layout,
                                                        const TextRenderStyle& style = {}) const;
    [[nodiscard]] PreparedText prepareUtf8(std::string_view utf8Text,
                                           const TextSystemBuildOptions& options = {}) const;
    [[nodiscard]] PreparedText prepareUtf8(std::u8string_view utf8Text,
                                           const TextSystemBuildOptions& options = {}) const;

    [[nodiscard]] const IBidiResolver& bidiResolver() const noexcept;
    [[nodiscard]] const ITextShaper& shaper() const noexcept;
    [[nodiscard]] const IGlyphMetricsProvider& glyphMetricsProvider() const noexcept;

  private:
    BasicBidiResolver mBidiResolver;
    BasicArabicShaper mTextShaper;
    InternalGlyphProvider mGlyphProvider;
    TextLayouter mLayouter;
};

} // namespace textsys
